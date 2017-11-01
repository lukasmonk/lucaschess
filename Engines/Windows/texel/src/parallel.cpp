/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * parallel.cpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#include "parallel.hpp"
#include "numa.hpp"
#include "cluster.hpp"
#include "clustertt.hpp"
#include "search.hpp"
#include "tbprobe.hpp"
#include "history.hpp"
#include "killerTable.hpp"
#include "textio.hpp"
#include "treeLogger.hpp"
#include "util/logger.hpp"

#include <cmath>
#include <cassert>


void
Notifier::notify() {
    {
        std::lock_guard<std::mutex> L(mutex);
        notified = true;
    }
    cv.notify_all();
}

void
Notifier::wait(int timeOutMs) {
    std::unique_lock<std::mutex> L(mutex);
    if (timeOutMs == -1) {
        while (!notified)
            cv.wait(L);
    } else {
        if (!notified)
            cv.wait_for(L, std::chrono::milliseconds(timeOutMs));
    }
    notified = false;
}

// ----------------------------------------------------------------------------

Communicator::Communicator(Communicator* parent, TranspositionTable& tt)
    : parent(parent) {
    ctt = make_unique<ClusterTT>(tt);
    if (parent)
        parent->addChild(this);
}

Communicator::~Communicator() {
    if (parent)
        parent->removeChild(this);
}

ClusterTT&
Communicator::getCTT() {
    return *ctt;
}

void
Communicator::addChild(Communicator* child) {
    std::lock_guard<std::mutex> L(mutex);
    children.push_back(child);
}

void
Communicator::removeChild(Communicator* child) {
    std::lock_guard<std::mutex> L(mutex);
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end());
}

void
Communicator::sendAssignThreads(int nThreadsThisNode,
                                const std::vector<int>& nThreadsChildren) {
    int threadOffs = Cluster::instance().getGlobalThreadOffset() + nThreadsThisNode;
    for (auto& c : children) {
        int childNo = c->clusterChildNo();
        if (childNo >= 0) {
            int nt = nThreadsChildren[childNo];
            c->doSendAssignThreads(nt, threadOffs);
            threadOffs += nt;
        }
    }
}

void
Communicator::forwardAssignThreads(int nThreads, int firstThreadNo) {
    for (auto& c : children)
        c->doSendAssignThreads(nThreads, firstThreadNo);
}

void
Communicator::sendInitSearch(const Position& pos,
                             const std::vector<U64>& posHashList, int posHashListSize,
                             bool clearHistory) {
    nodesSearched = 0;
    tbHits = 0;
    for (auto& c : children)
        c->doSendInitSearch(pos, posHashList, posHashListSize, clearHistory);
}

void
Communicator::sendStartSearch(int jobId, const SearchTreeInfo& sti,
                              int alpha, int beta, int depth) {
    for (auto& c : children)
        c->doSendStartSearch(jobId, sti, alpha, beta, depth);
}

void
Communicator::sendStopSearch() {
    stopAckWaitSelf = true;
    stopAckWaitChildren = children.size();
    notifyThread();
    for (auto& c : children)
        c->doSendStopSearch();
}

void
Communicator::sendSetParam(const std::string& name, const std::string& value, bool toAll) {
    for (auto& c : children)
        if (toAll || c->clusterChildNo() >= 0)
            c->doSendSetParam(name, value);
}

void
Communicator::sendQuit() {
    quitAckWaitChildren = children.size();
    if (quitAckWaitChildren > 0) {
        for (auto& c : children)
            c->doSendQuit();
    } else {
        quitAckWaitChildren = 1;
        sendQuitAck();
    }
}


void
Communicator::sendReportResult(int jobId, int score) {
    if (parent)
        parent->doSendReportResult(jobId, score);
}

void
Communicator::sendReportStats(S64 nodesSearched, S64 tbHits, bool propagate) {
    if (parent && propagate) {
        retrieveStats(nodesSearched, tbHits);
        parent->doSendReportStats(nodesSearched, tbHits);
    } else {
        doSendReportStats(nodesSearched, tbHits);
    }
}

void
Communicator::sendStopAck(bool child) {
    if (child) {
        stopAckWaitChildren--;
    } else {
        if (!stopAckWaitSelf)
            return;
        stopAckWaitSelf = false;
    }
    if (hasStopAck() && parent)
        parent->doSendStopAck();
}

void
Communicator::forwardStopAck() {
    if (parent)
        parent->doSendStopAck();
}

void
Communicator::sendQuitAck() {
    quitAckWaitChildren--;
    if (hasQuitAck()) {
        if (parent)
            parent->doSendQuitAck();
        else
            notifyThread();
    }
}

void
Communicator::forwardQuitAck() {
    if (parent)
        parent->doSendQuitAck();
}

void
Communicator::poll(CommandHandler& handler) {
    for (int pass = 0; pass < 2; pass++) {
        if (parent)
            parent->doPoll(pass);
        for (auto& c : children)
            c->doPoll(pass);
    }

    while (true) {
        std::unique_lock<std::mutex> L(mutex);
        if (cmdQueue.empty())
            break;
        std::shared_ptr<Command> cmd = cmdQueue.front();
        cmdQueue.pop_front();
        L.unlock();

        switch (cmd->type) {
        case CommandType::ASSIGN_THREADS: {
            const AssignThreadsCommand* aCmd = static_cast<const AssignThreadsCommand*>(cmd.get());
            handler.assignThreads(aCmd->nThreads, aCmd->firstThreadNo);
            break;
        }
        case CommandType::INIT_SEARCH: {
            const InitSearchCommand* iCmd = static_cast<const InitSearchCommand*>(cmd.get());
            Position pos;
            pos.deSerialize(iCmd->posData);
            handler.initSearch(pos, iCmd->posHashList, iCmd->posHashListSize, iCmd->clearHistory);
            break;
        }
        case CommandType::START_SEARCH: {
            const StartSearchCommand*  sCmd = static_cast<const StartSearchCommand*>(cmd.get());
            handler.startSearch(sCmd->jobId, sCmd->sti, sCmd->alpha, sCmd->beta, sCmd->depth);
            break;
        }
        case CommandType::STOP_SEARCH:
            handler.stopSearch();
            break;
        case CommandType::SET_PARAM: {
            const SetParamCommand* spCmd = static_cast<const SetParamCommand*>(cmd.get());
            handler.setParam(spCmd->name, spCmd->value);
            break;
        }
        case CommandType::QUIT:
            handler.quit();
            break;
        case CommandType::REPORT_RESULT:
            handler.reportResult(cmd->jobId, cmd->resultScore);
            break;
        case CommandType::STOP_ACK:
            handler.stopAck();
            break;
        case CommandType::QUIT_ACK:
            handler.quitAck();
            break;
        case CommandType::REPORT_STATS:
            break;
        case CommandType::TT_DATA:
        case CommandType::TT_ACK:
            assert(false);
        }
    }
}

// ----------------------------------------------------------------------------

using Serializer::putBytes;
using Serializer::getBytes;

U8*
Communicator::Command::toByteBuf(U8* buffer) const {
    return Serializer::serialize<64>(buffer, (int)type, jobId,
                                     resultScore, clearHistory);
}

const U8*
Communicator::Command::fromByteBuf(const U8* buffer) {
    int tmpType;
    buffer = Serializer::deSerialize<64>(buffer, tmpType, jobId,
                                         resultScore, clearHistory);
    type = (CommandType)tmpType;
    return buffer;
}

U8*
Communicator::AssignThreadsCommand::toByteBuf(U8* buffer) const {
    buffer = Command::toByteBuf(buffer);
    return Serializer::serialize<64>(buffer, nThreads, firstThreadNo);
    return buffer;
}

const U8*
Communicator::AssignThreadsCommand::fromByteBuf(const U8* buffer) {
    buffer = Command::fromByteBuf(buffer);
    buffer = Serializer::deSerialize<64>(buffer, nThreads, firstThreadNo);
    return buffer;
}

U8*
Communicator::InitSearchCommand::toByteBuf(U8* buffer) const {
    buffer = Command::toByteBuf(buffer);
    for (int i = 0; i < (int)COUNT_OF(posData.v); i++)
        buffer = putBytes(buffer, posData.v[i]);
    int len = posHashList.size();
    buffer = putBytes(buffer, len);
    for (int i = 0; i < len; i++)
        buffer = putBytes(buffer, posHashList[i]);
    buffer = putBytes(buffer, posHashListSize);
    return buffer;
}

const U8*
Communicator::InitSearchCommand::fromByteBuf(const U8* buffer) {
    buffer = Command::fromByteBuf(buffer);
    for (int i = 0; i < (int)COUNT_OF(posData.v); i++)
        buffer = getBytes(buffer, posData.v[i]);
    int len;
    buffer = getBytes(buffer, len);
    posHashList.resize(len);
    for (int i = 0; i < len; i++)
        buffer = getBytes(buffer, posHashList[i]);
    buffer = getBytes(buffer, posHashListSize);
    return buffer;
}

U8*
Communicator::StartSearchCommand::toByteBuf(U8* buffer) const {
    buffer = Command::toByteBuf(buffer);
    buffer = sti.serialize(buffer);
    buffer = Serializer::serialize<64>(buffer, alpha, beta, depth);
    return buffer;
}

const U8*
Communicator::StartSearchCommand::fromByteBuf(const U8* buffer) {
    buffer = Command::fromByteBuf(buffer);
    buffer = sti.deSerialize(buffer);
    buffer = Serializer::deSerialize<64>(buffer, alpha, beta, depth);
    return buffer;
}

U8*
Communicator::SetParamCommand::toByteBuf(U8* buffer) const {
    buffer = Command::toByteBuf(buffer);
    int len1 = name.length();
    int len2 = value.length();
    buffer = Serializer::serialize<64>(buffer, len1, len2);
    for (int i = 0; i < len1; i++)
        buffer = putBytes(buffer, name[i]);
    for (int i = 0; i < len2; i++)
        buffer = putBytes(buffer, value[i]);
    return buffer;
}

const U8*
Communicator::SetParamCommand::fromByteBuf(const U8* buffer) {
    buffer = Command::fromByteBuf(buffer);
    int len1, len2;
    buffer = Serializer::deSerialize<64>(buffer, len1, len2);
    name.resize(len1);
    for (int i = 0; i < len1; i++)
        buffer = getBytes(buffer, name[i]);
    value.resize(len2);
    for (int i = 0; i < len2; i++)
        buffer = getBytes(buffer, value[i]);
    return buffer;
}

U8*
Communicator::ReportStatsCommand::toByteBuf(U8* buffer) const {
    buffer = Command::toByteBuf(buffer);
    buffer = Serializer::serialize<64>(buffer, nodesSearched, tbHits);
    return buffer;
}

const U8*
Communicator::ReportStatsCommand::fromByteBuf(const U8* buffer) {
    buffer = Command::fromByteBuf(buffer);
    buffer = Serializer::deSerialize<64>(buffer, nodesSearched, tbHits);
    return buffer;
}

std::unique_ptr<Communicator::Command>
Communicator::Command::createFromByteBuf(const U8* buffer) {
    std::unique_ptr<Command> cmd;
    int tmpType;
    getBytes(buffer, tmpType);
    CommandType type = (CommandType)tmpType;
    switch (type) {
    case ASSIGN_THREADS:
        cmd = make_unique<AssignThreadsCommand>();
        break;
    case INIT_SEARCH:
        cmd = make_unique<InitSearchCommand>();
        break;
    case START_SEARCH:
        cmd = make_unique<StartSearchCommand>();
        break;
    case STOP_SEARCH:
    case QUIT:
    case REPORT_RESULT:
    case STOP_ACK:
    case QUIT_ACK:
    case TT_DATA:
    case TT_ACK:
        cmd = make_unique<Command>();
        break;
    case SET_PARAM:
        cmd = make_unique<SetParamCommand>();
        break;
    case REPORT_STATS:
        cmd = make_unique<ReportStatsCommand>();
        break;
    }

    cmd->fromByteBuf(buffer);
    return cmd;
}

// ----------------------------------------------------------------------------

ThreadCommunicator::ThreadCommunicator(Communicator* parent, TranspositionTable& tt,
                                       Notifier& notifier, bool createTTReceiver)
    : Communicator(parent, tt), notifier(&notifier) {
    if (createTTReceiver)
        ttReceiver = Cluster::instance().createLocalTTReceiver(tt);
}

ThreadCommunicator::~ThreadCommunicator() {
}

TTReceiver*
ThreadCommunicator::getTTReceiver() {
    return ttReceiver.get();
}

void
ThreadCommunicator::setNotifier(Notifier& notifier) {
    this->notifier = &notifier;
}

void
ThreadCommunicator::doSendAssignThreads(int nThreads, int firstThreadNo) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<AssignThreadsCommand>(nThreads, firstThreadNo));
    notifier->notify();
}

void
ThreadCommunicator::doSendInitSearch(const Position& pos,
                                     const std::vector<U64>& posHashList, int posHashListSize,
                                     bool clearHistory) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<InitSearchCommand>(pos, posHashList, posHashListSize, clearHistory));
    notifier->notify();
}

void
ThreadCommunicator::doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                                      int alpha, int beta, int depth) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<StartSearchCommand>(jobId, sti, alpha, beta, depth));
    notifier->notify();
}

void
ThreadCommunicator::doSendStopSearch() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_SEARCH));
    notifier->notify();
}

void
ThreadCommunicator::doSendSetParam(const std::string& name, const std::string& value) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<SetParamCommand>(name, value));
    notifier->notify();
}

void
ThreadCommunicator::doSendQuit() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::QUIT));
    notifier->notify();
}

void
ThreadCommunicator::doSendReportResult(int jobId, int score) {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::REPORT_RESULT, jobId, score));
    notifier->notify();
}

void
ThreadCommunicator::doSendReportStats(S64 nodesSearched, S64 tbHits) {
    std::lock_guard<std::mutex> L(mutex);
    this->nodesSearched += nodesSearched;
    this->tbHits += tbHits;
}

void
ThreadCommunicator::retrieveStats(S64& nodesSearched, S64& tbHits) {
    std::lock_guard<std::mutex> L(mutex);
    nodesSearched += this->nodesSearched;
    tbHits += this->tbHits;
    this->nodesSearched = 0;
    this->tbHits = 0;
}

void
ThreadCommunicator::doSendStopAck() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_ACK));
    notifier->notify();
}

void
ThreadCommunicator::doSendQuitAck() {
    std::lock_guard<std::mutex> L(mutex);
    cmdQueue.push_back(std::make_shared<Command>(CommandType::QUIT_ACK));
    notifier->notify();
}

void
ThreadCommunicator::notifyThread() {
    notifier->notify();
}


// ----------------------------------------------------------------------------

WorkerThread::WorkerThread(int threadNo, Communicator* parentComm,
                           int numWorkers, TranspositionTable& tt)
    : threadNo(threadNo), numWorkers(numWorkers), terminate(false), tt(tt) {
    if (parentComm) {
        auto f = [this,parentComm]() {
            mainLoop(parentComm, false);
        };
        thread = make_unique<std::thread>(f);
    }
}

WorkerThread::~WorkerThread() {
    children.clear();
    terminate = true;
    threadNotifier.notify();
    if (thread)
        thread->join();
}

void
WorkerThread::createWorkers(int firstThreadNo, Communicator* parentComm,
                            int numWorkers, TranspositionTable& tt,
                            std::vector<std::shared_ptr<WorkerThread>>& children) {
    if (numWorkers <= 0) {
        children.clear();
        return;
    }

    const int maxChildren = 4;
    int numChildren = std::min(numWorkers, maxChildren);
    std::vector<int> newChildren;
    children.resize(numChildren);
    for (int i = 0; i < numChildren; i++) {
        int n = (numWorkers + numChildren - i - 1) / (numChildren - i);
        if (!children[i] || children[i]->getThreadNo() != firstThreadNo ||
                children[i]->getNumWorkers() != n) {
            children[i] = std::make_shared<WorkerThread>(firstThreadNo, parentComm, n, tt);
            newChildren.push_back(i);
        }
        firstThreadNo += n;
        numWorkers -= n;
    }

    for (int i : newChildren)
        children[i]->waitInitialized();
}

void
WorkerThread::waitInitialized() {
    initialized.wait();
}

void
WorkerThread::mainLoopCluster(std::unique_ptr<ThreadCommunicator>&& comm) {
    this->comm = std::move(comm);
    mainLoop(nullptr, true);
}

void
WorkerThread::mainLoop(Communicator* parentComm, bool cluster) {
    Numa::instance().bindThread(threadNo);
    if (!cluster) {
        comm = make_unique<ThreadCommunicator>(parentComm, tt, threadNotifier, threadNo == 0);
        Cluster::instance().connectClusterReceivers(comm.get());
        createWorkers(threadNo + 1, comm.get(), numWorkers - 1, tt, children);
    } else
        comm->setNotifier(threadNotifier);

    initialized.notify();

    CommHandler handler(*this);

    while (true) {
        threadNotifier.wait(Cluster::instance().isEnabled() ? 1 : -1);
        if (terminate)
            break;
        comm->poll(handler);
        if (comm->hasQuitAck())
            break;
        if (jobId != -1)
            doSearch(handler);
        comm->sendStopAck(false);
    }
}

void
WorkerThread::CommHandler::assignThreads(int nThreads, int firstThreadNo) {
    Cluster::instance().setGlobalThreadOffset(firstThreadNo);
    int nThreadsThisNode;
    std::vector<int> nThreadsChildren;
    Cluster::instance().assignThreads(nThreads, nThreadsThisNode, nThreadsChildren);
    wt.comm->sendAssignThreads(nThreadsThisNode, nThreadsChildren);
    wt.disabled = nThreadsThisNode < 1;
    WorkerThread::createWorkers(1, wt.comm.get(), nThreadsThisNode - 1, wt.tt, wt.children);
}

void
WorkerThread::CommHandler::initSearch(const Position& pos,
                                      const std::vector<U64>& posHashList, int posHashListSize,
                                      bool clearHistory) {
    wt.comm->sendInitSearch(pos, posHashList, posHashListSize, clearHistory);
    wt.pos = pos;
    wt.posHashList = posHashList;
    wt.posHashListSize = posHashListSize;
    wt.jobId = -1;

    wt.logFile = make_unique<TreeLogger>();
    wt.logFile->open("/home/petero/treelog.dmp", wt.threadNo);
    wt.rootNodeIdx = wt.logFile->logPosition(pos);
    if (wt.kt)
        wt.kt->clear();
    if (wt.ht) {
        if (clearHistory)
            wt.ht->init();
        else
            wt.ht->reScale();
    }
}

void
WorkerThread::CommHandler::startSearch(int jobId, const SearchTreeInfo& sti,
                                       int alpha, int beta, int depth) {
    if (wt.disabled)
        return;
    wt.comm->sendStartSearch(jobId, sti, alpha, beta, depth);
    wt.sti = sti;
    wt.jobId = jobId;
    wt.alpha = alpha;
    wt.beta = beta;
    wt.depth = depth;
    wt.hasResult = false;
}

void
WorkerThread::CommHandler::stopSearch() {
    wt.comm->sendStopSearch();
    wt.jobId = -1;
}

void
WorkerThread::CommHandler::setParam(const std::string& name, const std::string& value) {
    wt.comm->sendSetParam(name, value);
    Parameters::instance().set(name, value);
}

void
WorkerThread::CommHandler::quit() {
    if (wt.getThreadNo() == 0)
        wt.comm->sendQuit();
    else {
        wt.comm->forwardQuitAck();
    }
}

void
WorkerThread::CommHandler::reportResult(int jobId, int score) {
    wt.sendReportResult(jobId, score);
}

void
WorkerThread::CommHandler::stopAck() {
    wt.comm->sendStopAck(true);
}

void
WorkerThread::CommHandler::quitAck() {
    wt.comm->sendQuitAck();
}

void
WorkerThread::sendReportResult(int jobId, int score) {
    if (!hasResult && (this->jobId == jobId)) {
        comm->sendReportResult(jobId, score);
        hasResult = true;
    }
}

void
WorkerThread::sendReportStats(S64 nodesSearched, S64 tbHits) {
    comm->sendReportStats(nodesSearched, tbHits, true);
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, int jobId, const Search& sc,
                      Communicator::CommandHandler& commHandler);

    /** Destructor. Report searched nodes to parent communicator. */
    ~ThreadStopHandler();

    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    bool shouldStop() override;

private:
    /** Report searched nodes since last call. */
    void reportNodes();

    WorkerThread& wt;
    const int jobId;
    const Search& sc;
    Communicator::CommandHandler& commHandler;
    int counter;             // Counts number of calls to shouldStop
    S64 lastReportedNodes;
    S64 lastReportedTbHits;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt, int jobId, const Search& sc,
                                     Communicator::CommandHandler& commHandler)
    : wt(wt), jobId(jobId), sc(sc), commHandler(commHandler), counter(0),
      lastReportedNodes(0), lastReportedTbHits(0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes();
}

bool
ThreadStopHandler::shouldStop() {
    wt.poll(commHandler);
    if (wt.shouldStop(jobId))
        return true;

    counter++;
    if (counter >= 100) {
        counter = 0;
        reportNodes();
    }

    return false;
}

void
ThreadStopHandler::reportNodes() {
    S64 totNodes = sc.getTotalNodesThisThread();
    S64 nodes = totNodes - lastReportedNodes;
    lastReportedNodes = totNodes;

    S64 totTbHits = sc.getTbHitsThisThread();
    S64 tbHits = totTbHits - lastReportedTbHits;
    lastReportedTbHits = totTbHits;

    wt.sendReportStats(nodes, tbHits);
}

void
WorkerThread::doSearch(CommHandler& commHandler) {
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = make_unique<KillerTable>();
    if (!ht)
        ht = make_unique<History>();

    using namespace SearchConst;
    int initExtraDepth = 0;
    for (int extraDepth = initExtraDepth; ; extraDepth++) {
        Search::SearchTables st(comm->getCTT(), *kt, *ht, *et);
        Position pos(this->pos);

        UndoInfo ui;
        pos.makeMove(sti.currentMove, ui);

        posHashList[posHashListSize++] = pos.zobristHash();
        Search sc(pos, posHashList, posHashListSize, st, *comm, *logFile);
        posHashListSize--;
        sc.setThreadNo(threadNo);
        sc.initSearchTreeInfo();
        const int minProbeDepth = TBProbe::tbEnabled() ? UciParams::minProbeDepth->getIntPar() : MAX_SEARCH_DEPTH;
        sc.setMinProbeDepth(minProbeDepth);

        auto stopHandler = make_unique<ThreadStopHandler>(*this, jobId, sc, commHandler);
        sc.setStopHandler(std::move(stopHandler));

        int ply = 1;
        sc.setSearchTreeInfo(ply-1, sti, rootNodeIdx);
        bool inCheck = MoveGen::inCheck(pos);
        U64 nodeIdx = logFile->peekNextNodeIdx();
        try {
            int searchDepth = std::min(depth + extraDepth, MAX_SEARCH_DEPTH);
            int captSquare = -1;
            int score = sc.negaScout(true, alpha, beta, ply, searchDepth, captSquare, inCheck);
            sendReportResult(jobId, score);
            if (searchDepth >= MAX_SEARCH_DEPTH) {
                jobId = -1;
                break;
            }
        } catch (const Search::StopSearch&) {
            logFile->logNodeEnd(nodeIdx, UNKNOWN_SCORE, TType::T_EMPTY,
                                UNKNOWN_SCORE, this->pos.historyHash());
            break;
        }
    }
}
