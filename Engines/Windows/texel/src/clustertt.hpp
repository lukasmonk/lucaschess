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
 * clustertt.hpp
 *
 *  Created on: Apr 13, 2017
 *      Author: petero
 */

#ifndef CLUSTERTT_HPP_
#define CLUSTERTT_HPP_

#include "transpositionTable.hpp"

#include <mutex>
#ifdef CLUSTER
#include <mpi.h>

/** A receiver of transposition table changes. */
class TTReceiver {
public:
    virtual ~TTReceiver() {}

    /** A batch of changes to a transposition table. */
    struct ChangeBatch {
        TranspositionTable::TTEntry ent[16];
        int nEnts = 0;
    };

    /** Apply chunk of TT changes, return minDepth, thread safe. */
    virtual int applyChunk(const ChangeBatch& changes) = 0;
};


/** Wrapper around a TranspositionTable object that also forwards
 *  transposition table changes to neighboring cluster nodes. */
class ClusterTT {
public:
    ClusterTT(TranspositionTable& tt);

    void addReceiver(TTReceiver* receiver);

    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore, bool busy = false);
    void setBusy(const TranspositionTable::TTEntry& ent, int ply);
    void probe(U64 key, TranspositionTable::TTEntry& result);
    void prefetch(U64 key);
    void extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv);
    std::string extractPV(const Position& posIn);
    int getHashFull() const;
    bool updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis);

    const TranspositionTable& getTT() const;
    void insert(const TranspositionTable::TTEntry& ent);
    void flush();

private:
    TranspositionTable& tt;
    int minDepth; // Smallest minDepth among all receivers

    void clusterInsert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore, bool busy);


    struct ReceiverData {
        ReceiverData(TTReceiver* receiver) : receiver(receiver) {}
        int minDepth = 0;
        TTReceiver::ChangeBatch changes;
        TTReceiver* receiver;
    };
    std::vector<ReceiverData> receivers;
};

/** Stores transposition table changes in a local transposition table. */
class LocalTTReceiver : public TTReceiver {
public:
    LocalTTReceiver(TranspositionTable& tt);

    int applyChunk(const ChangeBatch& changes) override;
private:
    TranspositionTable& tt;
};

/** Forwards transposition table changes to neighboring cluster node. */
class ClusterTTReceiver : public TTReceiver {
public:
    ClusterTTReceiver(int cmdType, int peerRank, ClusterTT& ctt);

    /** Set/clear disabled status. */
    void setDisabled(bool d);

    /** Add a chunk of changes to the internal buffer. */
    int applyChunk(const ChangeBatch& changes) override;

    /** Initiate a send request if there is any data to send.
     *  @return True if a send request was initiated, false otherwise. */
    bool sendBuffer(MPI_Request& sendReq);

    /** Process received data. */
    void receiveBuffer(const U8* buf, int len);

    /** Process TT data ack. */
    void ttAck(int nAcks);

private:
    void initBuf();

    const int cmdType;
    const int peerRank;
    ClusterTT& ctt;

    std::mutex mutex;
    int minDepth = 0;
    bool full = false;
    bool disabled = false;
    int nSendSlots = 16; // Number of TT data packets allowed to be "in flight"
    struct Buffer {
        int size = 0;
        std::array<U8, SearchConst::MAX_CLUSTER_BUF_SIZE> data;
    };
    Buffer* currBuf;
    Buffer buffer[2];
};


inline void
ClusterTT::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore, bool busy) {
    tt.insert(key, sm, type, ply, depth, evalScore, busy);
    if (depth >= minDepth)
        clusterInsert(key, sm, type, ply, depth, evalScore, busy);
}

inline void
ClusterTT::setBusy(const TranspositionTable::TTEntry& ent, int ply) {
    U64 key = ent.getKey();
    int type = ent.getType();
    int depth = ent.getDepth();
    int evalScore = ent.getEvalScore();
    Move sm;
    ent.getMove(sm);
    sm.setScore(ent.getScore(ply));
    insert(key, sm, type, ply, depth, evalScore, true);
}

inline void
ClusterTT::probe(U64 key, TranspositionTable::TTEntry& result) {
    tt.probe(key, result);
}

inline void
ClusterTT::prefetch(U64 key) {
    tt.prefetch(key);
}

inline void
ClusterTT::extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv) {
    tt.extractPVMoves(rootPos, mFirst, pv);
}

inline std::string
ClusterTT::extractPV(const Position& posIn) {
    return tt.extractPV(posIn);
}

inline int
ClusterTT::getHashFull() const {
    return tt.getHashFull();
}

inline bool
ClusterTT::updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis) {
    return tt.updateTB(pos, maxTimeMillis);
}

inline const TranspositionTable&
ClusterTT::getTT() const {
    return tt;
}

inline void
ClusterTTReceiver::setDisabled(bool d) {
    disabled = d;
}

#else
class TTReceiver {
public:
    virtual ~TTReceiver() {}
};
class ClusterTT {
public:
    ClusterTT(TranspositionTable& tt) : tt(tt) {}
    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore, bool busy = false) {
        tt.insert(key, sm, type, ply, depth, evalScore, busy);
    }
    void setBusy(const TranspositionTable::TTEntry& ent, int ply) {
        tt.setBusy(ent, ply);
    }
    void probe(U64 key, TranspositionTable::TTEntry& result) {
        tt.probe(key, result);
    }
    void prefetch(U64 key) {
        tt.prefetch(key);
    }
    void extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv) {
        tt.extractPVMoves(rootPos, mFirst, pv);
    }
    std::string extractPV(const Position& posIn) {
        return tt.extractPV(posIn);
    }
    int getHashFull() const {
        return tt.getHashFull();
    }
    bool updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis) {
        return tt.updateTB(pos, maxTimeMillis);
    }
    const TranspositionTable& getTT() const {
        return tt;
    }
private:
    TranspositionTable& tt;
};
#endif

#endif
