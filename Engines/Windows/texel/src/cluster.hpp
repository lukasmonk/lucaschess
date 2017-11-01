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
 * cluster.hpp
 *
 *  Created on: Mar 12, 2017
 *      Author: petero
 */

#ifndef CLUSTER_HPP_
#define CLUSTER_HPP_

#include "parallel.hpp"
#ifdef CLUSTER
#include <mpi.h>
#endif

#include <vector>


#ifdef CLUSTER

class TTReceiver;
class ClusterTTReceiver;

class Cluster {
public:
    /** Get the singleton instance. */
    static Cluster& instance();

    /** Initialize cluster processes. */
    void init(int* argc, char*** argv);

    /** Terminate cluster processes. */
    void finalize();

    /** Return true if this is the master cluster node. (rank 0) */
    bool isMasterNode() const;

    /** Return true if there is more than one cluster node. */
    bool isEnabled() const;

    /** Create a LocalTTReceiver object. */
    std::unique_ptr<TTReceiver> createLocalTTReceiver(TranspositionTable& tt);

    /** Create Communicator to communicate with cluster parent node. */
    Communicator* createParentCommunicator(TranspositionTable& tt);

    /** Create Communicators to communicate with cluster child nodes. */
    void createChildCommunicators(Communicator* mainThreadComm, TranspositionTable& tt);

    void connectAllReceivers(Communicator* comm);
    void connectClusterReceivers(Communicator* comm);

    /** Assign numThreads threads to this node and child nodes so that
     *  available cores and hardware threads are utilized in a good way. */
    void assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren);

    /** Get/set the offset between node local thread number and global thread number.
     *  globalThreadNo = globalThreadOffset + locaThreadNo */
    int getGlobalThreadOffset() const;
    void setGlobalThreadOffset(int offs);

    /** Return callers node number within the cluster. */
    int getNodeNumber() const;

private:
    Cluster();

    /** Return number of nodes in the cluster. */
    int getNumberOfNodes() const;

    /** Return the parent node number, or -1 if this is the root node. */
    int getParentNode() const;

    /** Return child node numbers. */
    const std::vector<int>& getChildNodes() const;

    /** Compute parent and child nodes. */
    void computeNeighbors();

    /** Compute number of cores/threads for this node and all child nodes. */
    void computeConcurrency();

    struct Concurrency {
        Concurrency(int c = 1, int t = 1) : cores(c), threads(t) {}
        int cores;    // Number of available cores
        int threads;  // Number of available hardware threads
    };

    /** Compute hardware concurrency for this node. */
    void computeThisConcurrency(Concurrency& concurrency) const;


    int rank = 0;
    int size = 1;
    int globalThreadOffs = 0;

    int parent = -1;
    std::vector<int> children;

    std::unique_ptr<Communicator> clusterParent;
    std::vector<std::unique_ptr<Communicator>> clusterChildren;

    Concurrency thisConcurrency;
    std::vector<std::vector<Concurrency>> childConcurrency;  // [childNo][level]
};

class MPICommunicator : public Communicator {
public:
    MPICommunicator(Communicator* parent, TranspositionTable& tt,
                    int myRank, int peerRank, int childNo);

    TTReceiver* getTTReceiver() override;

    int clusterChildNo() const override;

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

    void mpiSend();

    void doPoll(int pass) override;

    void notifyThread() override;

private:
    void mpiRecv();

    const int myRank;
    const int peerRank;
    const int childNo;

    bool sendBusy = false;
    MPI_Request sendReq;

    bool recvBusy = false;
    MPI_Request recvReq;

    std::unique_ptr<ClusterTTReceiver> ttReceiver;

    bool quitFlag = false;

    std::array<U8,SearchConst::MAX_CLUSTER_BUF_SIZE> sendBuf;
    std::array<U8,SearchConst::MAX_CLUSTER_BUF_SIZE> recvBuf;
};

inline bool
Cluster::isMasterNode() const {
    return getNodeNumber() == 0;
}

inline bool
Cluster::isEnabled() const {
    return getNumberOfNodes() > 1;
}

inline int
Cluster::getGlobalThreadOffset() const {
    return globalThreadOffs;
}

inline void
Cluster::setGlobalThreadOffset(int offs) {
    globalThreadOffs = offs;
}

inline int
Cluster::getNodeNumber() const {
    return rank;
}

inline int
Cluster::getNumberOfNodes() const {
    return size;
}

inline int
Cluster::getParentNode() const {
    return parent;
}

inline const std::vector<int>&
Cluster::getChildNodes() const {
    return children;
}

inline int
MPICommunicator::clusterChildNo() const {
    return childNo;
}

#else
class Cluster {
public:
    static Cluster& instance();
    void init(int* argc, char*** argv) {}
    void finalize() {}
    bool isMasterNode() const { return true; }
    bool isEnabled() const { return false; }
    std::unique_ptr<TTReceiver> createLocalTTReceiver(TranspositionTable& tt);
    Communicator* createParentCommunicator(TranspositionTable& tt) { return nullptr; }
    void createChildCommunicators(Communicator* mainThreadComm, TranspositionTable& tt) {}
    void connectAllReceivers(Communicator* comm) {}
    void connectClusterReceivers(Communicator* comm) {}
    void assignThreads(int numThreads, int& threadsThisNode, std::vector<int>& threadsChildren) {
        threadsThisNode = numThreads;
        threadsChildren.clear();
    }
    int getGlobalThreadOffset() const { return 0; }
    void setGlobalThreadOffset(int offs) {}
};
#endif


#endif /* CLUSTER_HPP_ */
