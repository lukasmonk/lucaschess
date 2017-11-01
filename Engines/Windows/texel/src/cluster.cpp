/*
    Texel - A UCI chess engine.
    Copyright (C) 2017  Peter Österlund, peterosterlund2@gmail.com

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
 * cluster.cpp
 *
 *  Created on: Mar 12, 2017
 *      Author: petero
 */

#include "cluster.hpp"
#include "clustertt.hpp"
#include "numa.hpp"
#include "util/logger.hpp"
#include <thread>
#include <iostream>


Cluster&
Cluster::instance() {
    static Cluster inst;
    return inst;
}

std::unique_ptr<TTReceiver>
Cluster::createLocalTTReceiver(TranspositionTable& tt) {
#ifdef CLUSTER
    return make_unique<LocalTTReceiver>(tt);
#else
    return nullptr;
#endif
}


#ifdef CLUSTER

Cluster::Cluster() {
}

void
Cluster::init(int* argc, char*** argv) {
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED)
        return;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    computeNeighbors();
    computeConcurrency();
}

void
Cluster::finalize() {
    MPI_Finalize();
}

void
Cluster::computeNeighbors() {
    const int maxChildren = 4;
    int n = getNodeNumber();
    parent = n > 0 ? (n - 1) / maxChildren : -1;

    for (int i = 0; i < maxChildren; i++) {
        int c = n * maxChildren + i + 1;
        if (c < getNumberOfNodes())
            children.push_back(c);
    }
}

void
Cluster::computeConcurrency() {
    computeThisConcurrency(thisConcurrency);

    const int nChild = children.size();
    int nChildLevels = 0;
    for (int c = 0; c < nChild; c++) {
        MPI_Status status;
        MPI_Probe(children[c], 0, MPI_COMM_WORLD, &status);
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        std::vector<int> buf(count);
        MPI_Recv(&buf[0], count, MPI_INT, children[c], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::vector<Concurrency> childConcur;
        int nLev = count / 2;
        for (int i = 0; i < nLev; i++)
            childConcur.emplace_back(buf[2*i], buf[2*i+1]);
        childConcurrency.push_back(std::move(childConcur));
        nChildLevels = std::max(nChildLevels, nLev);
    }
    if (parent != -1) {
        int count = (nChildLevels + 1) * 2;
        std::vector<int> buf(count);
        buf[0] = thisConcurrency.cores;
        buf[1] = thisConcurrency.threads;
        for (int lev = 0; lev < nChildLevels; lev++) {
            int nc = 0, nt = 0;
            for (int c = 0; c < nChild; c++) {
                if (lev < (int)childConcurrency[c].size()) {
                    nc += childConcurrency[c][lev].cores;
                    nt += childConcurrency[c][lev].threads;
                }
            }
            buf[2*lev+2] = nc;
            buf[2*lev+3] = nt;
        }
        MPI_Send(&buf[0], count, MPI_INT, parent, 0, MPI_COMM_WORLD);
    }
    if (getNodeNumber() == 0) {
        int nc = thisConcurrency.cores;
        int nt = thisConcurrency.threads;
        for (auto& cc : childConcurrency) {
            for (auto& c : cc) {
                nc += c.cores;
                nt += c.threads;
            }
        }
        std::cout << "info string cores:" << nc << " threads:" << nt << std::endl;
    }
}

void
Cluster::computeThisConcurrency(Concurrency& concurrency) const {
    int nodes;
    Numa::instance().getConcurrency(nodes, concurrency.cores, concurrency.threads);
}

Communicator*
Cluster::createParentCommunicator(TranspositionTable& tt) {
    if (getParentNode() == -1)
        return nullptr;
    clusterParent = make_unique<MPICommunicator>(nullptr, tt, getNodeNumber(), getParentNode(), -1);
    return clusterParent.get();
}

void
Cluster::createChildCommunicators(Communicator* mainThreadComm, TranspositionTable& tt) {
    std::vector<int> childRanks = getChildNodes();
    int n = childRanks.size();
    for (int i = 0; i < n; i++) {
        int peerRank = childRanks[i];
        auto comm = make_unique<MPICommunicator>(mainThreadComm, tt, getNodeNumber(), peerRank, i);
        clusterChildren.push_back(std::move(comm));
    }
}

void
Cluster::connectAllReceivers(Communicator* comm) {
    connectClusterReceivers(comm);

    auto connectLocalReceiver = [comm](Communicator* src) {
        if (src)
            src->getCTT().addReceiver(comm->getTTReceiver());
    };
    connectLocalReceiver(clusterParent.get());
    for (auto& c : clusterChildren)
        connectLocalReceiver(c.get());

    auto connectOtherClusterReceivers = [this](Communicator* src) {
        auto connectReceiver = [](Communicator* src, Communicator* dst) {
            if (src && dst && src != dst)
                src->getCTT().addReceiver(dst->getTTReceiver());
        };
        connectReceiver(src, clusterParent.get());
        for (auto& c : clusterChildren)
            connectReceiver(src, c.get());
    };
    connectOtherClusterReceivers(clusterParent.get());
    for (auto& c : clusterChildren)
        connectOtherClusterReceivers(c.get());
}

void
Cluster::connectClusterReceivers(Communicator* comm) {
    auto connectReceiver = [comm](Communicator* dest) {
        if (dest)
            comm->getCTT().addReceiver(dest->getTTReceiver());
    };
    connectReceiver(clusterParent.get());
    for (auto& c : clusterChildren)
        connectReceiver(c.get());
}

void
Cluster::assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren) {
    int nTotalCores = thisConcurrency.cores;
    int nTotalThreads = thisConcurrency.threads;
    const int nChild = childConcurrency.size();
    threadsThisNode = 0;
    threadsChildren.assign(nChild, 0);
    int numChildLevels = 0;
    std::vector<int> htChildren(nChild, 0);
    for (int c = 0; c < nChild; c++) {
        const std::vector<Concurrency>& childConcur = childConcurrency[c];
        int nLev = childConcur.size();
        for (int lev = 0; lev < nLev; lev++) {
            int nc = childConcur[lev].cores;
            int nt = childConcur[lev].threads;
            nTotalCores += nc;
            nTotalThreads += nt;
            htChildren[c] += nt - nc;
        }
        numChildLevels = std::max(numChildLevels, nLev);
    }

    const int nOverCommit = numThreads / nTotalThreads;
    numThreads %= nTotalThreads;

    // Assign threads to cores using breadth first
    if (numThreads > 0) {
        int t = std::min(thisConcurrency.cores, numThreads);
        threadsThisNode += t;
        numThreads -= t;
    }
    for (int lev = 0; lev < numChildLevels && numThreads > 0; lev++) {
        for (int c = 0; c < nChild && numThreads > 0; c++) {
            if (lev < (int)childConcurrency[c].size()) {
                int t = std::min(childConcurrency[c][lev].cores, numThreads);
                threadsChildren[c] += t;
                numThreads -= t;
            }
        }
    }

    // Assign threads to hardware threads proportionally
    int htThisNode = thisConcurrency.threads - thisConcurrency.cores;
    int htRemain = htThisNode;
    for (int c = 0; c < nChild; c++)
        htRemain += htChildren[c];

    if (numThreads > 0) {
        int t = (numThreads * htThisNode + htRemain - 1) / htRemain;
        threadsThisNode += t;
        numThreads -= t;
        htRemain -= htThisNode;
    }
    for (int c = 0; c < nChild && numThreads > 0; c++) {
        int t = (numThreads * htChildren[c] + htRemain - 1) / htRemain;
        threadsChildren[c] += t;
        numThreads -= t;
        htRemain -= htChildren[c];
    }

    // Assign over-committed threads
    threadsThisNode += nOverCommit * thisConcurrency.threads;
    for (int c = 0; c < nChild; c++) {
        int concur = 0;
        for (auto& e : childConcurrency[c])
            concur += e.threads;
        threadsChildren[c] += nOverCommit * concur;
    }
}

// ----------------------------------------------------------------------------

MPICommunicator::MPICommunicator(Communicator* parent, TranspositionTable& tt,
                                 int myRank, int peerRank, int childNo)
    : Communicator(parent, tt), myRank(myRank), peerRank(peerRank), childNo(childNo) {
    ttReceiver = make_unique<ClusterTTReceiver>(CommandType::TT_DATA, peerRank, *ctt);
}

TTReceiver*
MPICommunicator::getTTReceiver() {
    return ttReceiver.get();
}

void
MPICommunicator::doSendAssignThreads(int nThreads, int firstThreadNo) {
    ttReceiver->setDisabled(nThreads == 0);
    cmdQueue.push_back(std::make_shared<AssignThreadsCommand>(nThreads, firstThreadNo));
    mpiSend();
}

void
MPICommunicator::doSendInitSearch(const Position& pos,
                                  const std::vector<U64>& posHashList, int posHashListSize,
                                  bool clearHistory) {
    cmdQueue.push_back(std::make_shared<InitSearchCommand>(pos, posHashList,
                                                           posHashListSize, clearHistory));
    mpiSend();
}

void
MPICommunicator::doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                                   int alpha, int beta, int depth) {
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<StartSearchCommand>(jobId, sti, alpha, beta, depth));
    mpiSend();
}

void
MPICommunicator::doSendStopSearch() {
    cmdQueue.erase(std::remove_if(cmdQueue.begin(), cmdQueue.end(),
                                  [](const std::shared_ptr<Command>& cmd) {
                                      return cmd->type == CommandType::START_SEARCH ||
                                             cmd->type == CommandType::STOP_SEARCH ||
                                             cmd->type == CommandType::REPORT_RESULT;
                                  }),
                   cmdQueue.end());
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_SEARCH));
    mpiSend();
}

void
MPICommunicator::doSendSetParam(const std::string& name, const std::string& value) {
    int s = name.length() + value.length() + 2 * sizeof(int);
    if (s + sizeof(Communicator::Command) < SearchConst::MAX_CLUSTER_BUF_SIZE) {
        cmdQueue.push_back(std::make_shared<SetParamCommand>(name, value));
        mpiSend();
    }
}

void
MPICommunicator::doSendQuit() {
    cmdQueue.push_back(std::make_shared<Command>(CommandType::QUIT));
    mpiSend();
}

void
MPICommunicator::doSendReportResult(int jobId, int score) {
    cmdQueue.push_back(std::make_shared<Command>(CommandType::REPORT_RESULT, jobId, score));
    mpiSend();
}

void
MPICommunicator::doSendReportStats(S64 nodesSearched, S64 tbHits) {
    bool done = false;
    for (std::shared_ptr<Command>& c : cmdQueue) {
        if (c->type == CommandType::REPORT_STATS) {
            ReportStatsCommand* rCmd = static_cast<ReportStatsCommand*>(c.get());
            rCmd->nodesSearched += nodesSearched;
            rCmd->tbHits += tbHits;
            done = true;
            break;
        }
    }
    if (!done)
        cmdQueue.push_back(std::make_shared<ReportStatsCommand>(nodesSearched, tbHits));
    mpiSend();
}

void
MPICommunicator::retrieveStats(S64& nodesSearched, S64& tbHits) {
    assert(false); // Not used
}

void
MPICommunicator::doSendStopAck() {
    cmdQueue.push_back(std::make_shared<Command>(CommandType::STOP_ACK));
    mpiSend();
}

void
MPICommunicator::doSendQuitAck() {
    cmdQueue.push_back(std::make_shared<Command>(CommandType::QUIT_ACK));
    mpiSend();
}

void
MPICommunicator::mpiSend() {
    for (int loop = 0; loop < 100; loop++) {
        if (sendBusy) {
            int flag;
            MPI_Test(&sendReq, &flag, MPI_STATUS_IGNORE);
            if (!flag)
                break;
            sendBusy = false;
        }
        if (cmdQueue.empty())
            break;
        std::shared_ptr<Command> cmd = cmdQueue.front();
        cmdQueue.pop_front();
        U8* buf = cmd->toByteBuf(&sendBuf[0]);
        int count = (int)(buf - &sendBuf[0]);
        MPI_Isend(&sendBuf[0], count, MPI_BYTE, peerRank, 0, MPI_COMM_WORLD, &sendReq);
        sendBusy = true;
    }

    if (!sendBusy) {
        if (ttReceiver->sendBuffer(sendReq))
            sendBusy = true;
    }
}

void
MPICommunicator::doPoll(int pass) {
    if (pass == 0)
        mpiRecv();
    if (pass == 1)
        mpiSend();
}

void
MPICommunicator::mpiRecv() {
    int nTTReceives = 0;
    for (int loop = 0; loop < 100; loop++) {
        if (recvBusy) {
            int flag;
            MPI_Status status;
            MPI_Test(&recvReq, &flag, &status);
            if (flag) {
                std::unique_ptr<Command> cmd = Command::createFromByteBuf(&recvBuf[0]);
                switch (cmd->type) {
                case CommandType::ASSIGN_THREADS: {
                    const AssignThreadsCommand* aCmd = static_cast<const AssignThreadsCommand*>(cmd.get());
                    forwardAssignThreads(aCmd->nThreads, aCmd->firstThreadNo);
                    break;
                }
                case CommandType::INIT_SEARCH: {
                    const InitSearchCommand* iCmd = static_cast<const InitSearchCommand*>(cmd.get());
                    Position pos;
                    pos.deSerialize(iCmd->posData);
                    sendInitSearch(pos, iCmd->posHashList, iCmd->posHashListSize, iCmd->clearHistory);
                    break;
                }
                case CommandType::START_SEARCH: {
                    const StartSearchCommand*  sCmd = static_cast<const StartSearchCommand*>(cmd.get());
                    sendStartSearch(sCmd->jobId, sCmd->sti, sCmd->alpha, sCmd->beta, sCmd->depth);
                    break;
                }
                case CommandType::STOP_SEARCH:
                    sendStopSearch();
                    break;
                case CommandType::SET_PARAM: {
                    const SetParamCommand* spCmd = static_cast<const SetParamCommand*>(cmd.get());
                    sendSetParam(spCmd->name, spCmd->value, true);
                    break;
                }
                case CommandType::QUIT:
                    sendQuit();
                    quitFlag = true;
                    break;
                case CommandType::REPORT_RESULT:
                    sendReportResult(cmd->jobId, cmd->resultScore);
                    break;
                case CommandType::STOP_ACK:
                    forwardStopAck();
                    break;
                case CommandType::QUIT_ACK:
                    forwardQuitAck();
                    quitFlag = true;
                    break;
                case CommandType::REPORT_STATS: {
                    const ReportStatsCommand* rCmd = static_cast<const ReportStatsCommand*>(cmd.get());
                    parent->sendReportStats(rCmd->nodesSearched, rCmd->tbHits, false);
                    break;
                }
                case CommandType::TT_DATA: {
                    int count;
                    MPI_Get_count(&status, MPI_BYTE, &count);
                    ttReceiver->receiveBuffer(&recvBuf[0], count);
                    nTTReceives++;
                    break;
                }
                case CommandType::TT_ACK:
                    ttReceiver->ttAck(cmd->resultScore);
                    break;
                }
                recvBusy = false;
            }
        }
        if (recvBusy || quitFlag)
            break;
        if (!recvBusy) {
            MPI_Irecv(&recvBuf[0], SearchConst::MAX_CLUSTER_BUF_SIZE,
                      MPI_BYTE, peerRank, 0, MPI_COMM_WORLD, &recvReq);
            recvBusy = true;
        }
    }
    if (nTTReceives > 0)
        cmdQueue.push_back(std::make_shared<Command>(CommandType::TT_ACK, -1, nTTReceives));
}

void
MPICommunicator::notifyThread() {
}

#endif // CLUSTER
