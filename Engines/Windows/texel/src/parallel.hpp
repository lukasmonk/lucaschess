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
 * parallel.hpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#ifndef PARALLEL_HPP_
#define PARALLEL_HPP_

#include "evaluate.hpp"
#include "searchUtil.hpp"
#include "constants.hpp"
#include "util/timeUtil.hpp"

#include <memory>
#include <vector>
#include <deque>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


class History;
class KillerTable;
class TranspositionTable;
class ClusterTT;
class TTReceiver;


class Notifier {
public:
    /** Set the condition. This method can be called by multiple threads. */
    void notify();

    /** Wait until notify has been called at least once since the last call
     *  to this method. The method should only be called by one thread.
     *  @param timeOutMs Maximum amount of time to wait, or -1 to wait indefinitely */
    void wait(int timeOutMs = -1);

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool notified = false;
};


/** Handles communication with parent and child threads. */
class Communicator {
public:
    explicit Communicator(Communicator* parent, TranspositionTable& tt);
    Communicator(const Communicator&) = delete;
    Communicator& operator=(const Communicator&) = delete;
    virtual ~Communicator();

    ClusterTT& getCTT();
    virtual TTReceiver* getTTReceiver() = 0;

    /** Add/remove a child communicator. */
    void addChild(Communicator* child);
    void removeChild(Communicator* child);

    /** Return which cluster child this communicator represents.
     * @return A number between 0 and N-1 where N is the number of cluster children
     *         this node's parent has.
     *         Return -1 if this communicator does not represent a cluster child. */
    virtual int clusterChildNo() const = 0;

    // Parent to child commands

    void sendAssignThreads(int nThreadsThisNode, const std::vector<int>& nThreadsChildren);
    void forwardAssignThreads(int nThreads, int firstThreadNo);

    void sendInitSearch(const Position& pos,
                        const std::vector<U64>& posHashList, int posHashListSize,
                        bool clearHistory);

    void sendStartSearch(int jobId, const SearchTreeInfo& sti,
                         int alpha, int beta, int depth);

    void sendStopSearch();

    void sendSetParam(const std::string& name, const std::string& value, bool toAll = false);

    /** Tell child cluster nodes to exit the program. */
    void sendQuit();


    // Child to parent commands

    void sendReportResult(int jobId, int score);

    void sendReportStats(S64 nodesSearched, S64 tbHits, bool propagate);

    void sendStopAck(bool child);
    /** Forward stop ack from cluster child. */
    void forwardStopAck();

    /** Tell parent cluster node that program is about to exit. */
    void sendQuitAck();
    /** Forward quit ack from cluster child. */
    void forwardQuitAck();

    /** Handler invoked when commands are received. */
    class CommandHandler {
    public:
        virtual void assignThreads(int nThreads, int firstThreadNo) {}
        virtual void initSearch(const Position& pos,
                                const std::vector<U64>& posHashList, int posHashListSize,
                                bool clearHistory) {}
        virtual void startSearch(int jobId, const SearchTreeInfo& sti,
                                 int alpha, int beta, int depth) {}
        virtual void stopSearch() {}
        virtual void setParam(const std::string& name, const std::string& value) {}
        virtual void quit() {}

        virtual void reportResult(int jobId, int score) {}
        virtual void stopAck() {}
        virtual void quitAck() {}
    };

    /** Check if a command has been received. */
    void poll(CommandHandler& handler);

    /** Return true if all child threads have acknowledged the stop command. */
    bool hasStopAck() const;

    /** Return true if all child threads have acknowledged the quit command. */
    bool hasQuitAck() const;

    /** Get number of of searched nodes/tbhits for all helper threads. */
    S64 getNumSearchedNodes() const;
    S64 getTbHits() const;

protected:
    virtual void doSendAssignThreads(int nThreads, int firstThreadNo) = 0;
    virtual void doSendInitSearch(const Position& pos,
                                  const std::vector<U64>& posHashList, int posHashListSize,
                                  bool clearHistory) = 0;
    virtual void doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                                   int alpha, int beta, int depth) = 0;
    virtual void doSendStopSearch() = 0;
    virtual void doSendSetParam(const std::string& name, const std::string& value) = 0;
    virtual void doSendQuit() = 0;

    virtual void doSendReportResult(int jobId, int score) = 0;
    virtual void doSendReportStats(S64 nodesSearched, S64 tbHits) = 0;
    virtual void retrieveStats(S64& nodesSearched, S64& tbHits) = 0;
    virtual void doSendStopAck() = 0;
    virtual void doSendQuitAck() = 0;

    virtual void doPoll(int pass) = 0;
    /** Notify corresponding search thread that something has happened. */
    virtual void notifyThread() = 0;

    Communicator* const parent;
    std::vector<Communicator*> children;
    std::unique_ptr<ClusterTT> ctt;

    enum CommandType {
        ASSIGN_THREADS,
        INIT_SEARCH,
        START_SEARCH,
        STOP_SEARCH,
        SET_PARAM,
        QUIT,

        REPORT_RESULT,
        STOP_ACK,
        QUIT_ACK,

        // Only used for cluster communication
        REPORT_STATS,
        TT_DATA,      // Transposition table data
        TT_ACK        // Acknowledge one or more TT_DATA messages
    };
    struct Command {
        Command() {}
        Command(CommandType type, int jobId = -1, int resultScore = 0, bool clearHistory = false)
            : type(type), jobId(jobId),
              resultScore(resultScore), clearHistory(clearHistory) {}
        virtual U8* toByteBuf(U8* buffer) const;
        virtual const U8* fromByteBuf(const U8* buffer);
        static std::unique_ptr<Command> createFromByteBuf(const U8* buffer);

        CommandType type { QUIT };
        int jobId = 0;
        int resultScore = 0;
        bool clearHistory = false;
    };
    struct AssignThreadsCommand : public Command {
        AssignThreadsCommand() {}
        AssignThreadsCommand(int nThreads, int firstThreadNo)
            : Command(ASSIGN_THREADS), nThreads(nThreads), firstThreadNo(firstThreadNo) {
        }
        U8* toByteBuf(U8* buffer) const override;
        const U8* fromByteBuf(const U8* buffer) override;

        int nThreads = 0;
        int firstThreadNo = 0;
    };
    struct InitSearchCommand : public Command {
        InitSearchCommand() {}
        InitSearchCommand(const Position& pos,const std::vector<U64>& posHashList,
                          int posHashListSize, bool clearHistory)
            : Command(INIT_SEARCH, -1, 0, clearHistory),
              posHashList(posHashList), posHashListSize(posHashListSize) {
            pos.serialize(posData);
        }
        U8* toByteBuf(U8* buffer) const override;
        const U8* fromByteBuf(const U8* buffer) override;

        Position::SerializeData posData;
        std::vector<U64> posHashList;
        int posHashListSize = 0;
    };
    struct StartSearchCommand : public Command {
        StartSearchCommand() {}
        StartSearchCommand(int jobId, const SearchTreeInfo& sti,
                           int alpha, int beta, int depth)
            : Command(START_SEARCH, jobId),
              sti(sti), alpha(alpha), beta(beta), depth(depth) {}
        U8* toByteBuf(U8* buffer) const override;
        const U8* fromByteBuf(const U8* buffer) override;

        SearchTreeInfo sti;
        int alpha = 0;
        int beta = 0;
        int depth = 0;
    };
    struct SetParamCommand : public Command {
        SetParamCommand() {}
        SetParamCommand(const std::string& name, const std::string& value)
            : Command(SET_PARAM), name(name), value(value) {}
        U8* toByteBuf(U8* buffer) const override;
        const U8* fromByteBuf(const U8* buffer) override;

        std::string name;
        std::string value;
    };
    struct ReportStatsCommand : public Command {
        ReportStatsCommand() {}
        ReportStatsCommand(S64 nodesSearched, S64 tbHits)
            : Command(REPORT_STATS), nodesSearched(nodesSearched), tbHits(tbHits) {
        }
        U8* toByteBuf(U8* buffer) const override;
        const U8* fromByteBuf(const U8* buffer) override;

        S64 nodesSearched = 0;
        S64 tbHits = 0;
    };
    std::deque<std::shared_ptr<Command>> cmdQueue;

    std::mutex mutex;
    bool stopAckWaitSelf = false;
    int stopAckWaitChildren = 0;
    int quitAckWaitChildren = -1;

    std::atomic<S64> nodesSearched{0};
    std::atomic<S64> tbHits{0};
};


/** Handles communication between search threads within the same process. */
class ThreadCommunicator : public Communicator {
public:
    ThreadCommunicator(Communicator* parent, TranspositionTable& tt,
                       Notifier& notifier, bool createTTReceiver);
    ~ThreadCommunicator();

    TTReceiver* getTTReceiver() override;

    void setNotifier(Notifier& notifier);

    int clusterChildNo() const override;

protected:
    void doSendAssignThreads(int nThreads, int firstThreadNo) override;
    void doSendInitSearch(const Position& pos,
                          const std::vector<U64>& posHashList, int posHashListSize,
                          bool clearHistory) override;
    void doSendStartSearch(int jobId, const SearchTreeInfo& sti,
                           int alpha, int beta, int depth) override;
    void doSendStopSearch() override;
    void doSendSetParam(const std::string& name, const std::string& value) override;
    void doSendQuit() override;

    void doSendReportResult(int jobId, int score) override;
    void doSendReportStats(S64 nodesSearched, S64 tbHits) override;
    void retrieveStats(S64& nodesSearched, S64& tbHits) override;
    void doSendStopAck() override;
    void doSendQuitAck() override;

    void doPoll(int pass) override {}
    void notifyThread() override;

private:
    Notifier* notifier;
    std::unique_ptr<TTReceiver> ttReceiver;
};


/** Handles communication between search threads. */
class WorkerThread {
public:
    /** Constructor. */
    WorkerThread(int threadNo, Communicator* parentComm, int numWorkers,
                 TranspositionTable& tt);

    /** Destructor. Waits for thread to terminate. */
    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    /** Create numWorkers WorkerThread objects, arranged in a tree structure.
     *  parentComm is the Communicator corresponding to the already existing
     *  root node in that tree structure. The children to the root node are
     *  returned in the "children" variable. */
    static void createWorkers(int firstThreadNo, Communicator* parentComm,
                              int numWorkers, TranspositionTable& tt,
                              std::vector<std::shared_ptr<WorkerThread>>& children);

    /** Wait until all child workers have been initialized. */
    void waitInitialized();

    /** Send search result to parent. */
    void sendReportResult(int jobId, int score);

    /** Send node counters to parent. */
    void sendReportStats(S64 nodesSearched, S64 tbHits);

    /** Return thread number. The first worker thread is number 1. */
    int getThreadNo() const;

    /** Get number of worker threads in the tree rooted at this worker. */
    int getNumWorkers() const;

    /** Return true if the worker thread should not be searching. */
    bool shouldStop(int jobId) const;

    /** Handle commands received from other Communicator objects. */
    void poll(Communicator::CommandHandler& handler);

    /** Main loop for thread 0 on non-master cluster nodes. */
    void mainLoopCluster(std::unique_ptr<ThreadCommunicator>&& comm);

private:
    /** Thread main loop. */
    void mainLoop(Communicator* parentComm, bool cluster);

    class CommHandler : public Communicator::CommandHandler {
    public:
        explicit CommHandler(WorkerThread& wt);
        void assignThreads(int nThreads, int firstThreadNo) override;
        void initSearch(const Position& pos,
                        const std::vector<U64>& posHashList, int posHashListSize,
                        bool clearHistory) override;
        void startSearch(int jobId, const SearchTreeInfo& sti,
                         int alpha, int beta, int depth) override;
        void stopSearch() override;
        void setParam(const std::string& name, const std::string& value) override;
        void quit() override;
        void reportResult(int jobId, int score) override;
        void stopAck() override;
        void quitAck() override;
    private:
        WorkerThread& wt;
    };

    /** Run a search for the current search parameters. */
    void doSearch(CommHandler& commHandler);


    int threadNo;
    bool disabled = false; // True for not used cluster node
    std::unique_ptr<ThreadCommunicator> comm;
    std::unique_ptr<std::thread> thread;
    Notifier threadNotifier;
    std::vector<std::shared_ptr<WorkerThread>> children;
    const int numWorkers; // Number of worker threads including all child threads

    Notifier initialized;
    std::atomic<bool> terminate;

    std::unique_ptr<Evaluate::EvalHashTables> et;
    std::unique_ptr<KillerTable> kt;
    std::unique_ptr<History> ht;
    TranspositionTable& tt;

    std::unique_ptr<TreeLogger> logFile;
    U64 rootNodeIdx = 0;
    Position pos;
    SearchTreeInfo sti;
    std::vector<U64> posHashList;
    int posHashListSize = 0;
    int jobId = -1; // job ID or -1 when no active search job
    int alpha = 0;
    int beta = 0;
    int depth = -1;

    bool hasResult = false;
};


inline bool
Communicator::hasStopAck() const {
    return stopAckWaitChildren == 0 && !stopAckWaitSelf;
}

inline bool
Communicator::hasQuitAck() const {
    return quitAckWaitChildren == 0;
}

inline S64
Communicator::getNumSearchedNodes() const {
    return nodesSearched;
}

inline S64
Communicator::getTbHits() const {
    return tbHits;
}

inline int
ThreadCommunicator::clusterChildNo() const {
    return -1;
}

inline int
WorkerThread::getThreadNo() const {
    return threadNo;
}

inline int
WorkerThread::getNumWorkers() const {
    return numWorkers;
}

inline bool
WorkerThread::shouldStop(int jobId) const {
    return (this->jobId != jobId) || terminate;
}

inline void
WorkerThread::poll(Communicator::CommandHandler& handler) {
    comm->poll(handler);
}

inline
WorkerThread::CommHandler::CommHandler(WorkerThread& wt)
    : wt(wt) {
}

#endif /* PARALLEL_HPP_ */
