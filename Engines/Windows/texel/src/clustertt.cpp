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
 * clustertt.cpp
 *
 *  Created on: Apr 13, 2017
 *      Author: petero
 */

#include "clustertt.hpp"
#include "cluster.hpp"
#include "util/logger.hpp"
#include "treeLogger.hpp"

#include <limits.h>

#ifdef CLUSTER

ClusterTT::ClusterTT(TranspositionTable& tt)
    : tt(tt), minDepth(Cluster::instance().isEnabled() ? 0 : INT_MAX) {
}

void
ClusterTT::addReceiver(TTReceiver* receiver) {
    if (receiver)
        receivers.emplace_back(receiver);
}

void
ClusterTT::clusterInsert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore, bool busy) {
    TranspositionTable::TTEntry ent;
    ent.clear();
    ent.setMove(sm);
    ent.setKey(key);
    ent.setScore(sm.score(), ply);
    ent.setDepth(depth);
    ent.setBusy(busy);
    ent.setType(type);
    ent.setEvalScore(evalScore);
    insert(ent);
}

void
ClusterTT::insert(const TranspositionTable::TTEntry& ent) {
    const int depth = ent.getDepth();
    bool minDepthModified = false;
    for (ReceiverData& rd : receivers) {
        if (depth < rd.minDepth)
            continue;
        rd.changes.ent[rd.changes.nEnts++] = ent;
        if (rd.changes.nEnts >= (int)COUNT_OF(rd.changes.ent)) {
            int newDepth = rd.receiver->applyChunk(rd.changes);
            rd.changes.nEnts = 0;
            if (rd.minDepth != newDepth) {
                rd.minDepth = newDepth;
                minDepthModified = true;
            }
        }
    }
    if (minDepthModified) {
        minDepth = INT_MAX;
        for (ReceiverData& rd : receivers)
            minDepth = std::min(minDepth, rd.minDepth);
    }
}

void
ClusterTT::flush() {
    bool minDepthModified = false;
    for (ReceiverData& rd : receivers) {
        int newDepth = rd.receiver->applyChunk(rd.changes);
        rd.changes.nEnts = 0;
        if (rd.minDepth != newDepth) {
            rd.minDepth = newDepth;
            minDepthModified = true;
        }
    }
    if (minDepthModified) {
        minDepth = INT_MAX;
        for (ReceiverData& rd : receivers)
            minDepth = std::min(minDepth, rd.minDepth);
    }
}

// ----------------------------------------------------------------------------

LocalTTReceiver::LocalTTReceiver(TranspositionTable& tt)
    : tt(tt) {
}

int
LocalTTReceiver::applyChunk(const ChangeBatch& changes) {
    int n = changes.nEnts;
#ifdef HAS_PREFETCH
    for (int i = 0; i < n; i++)
        tt.prefetch(changes.ent[i].getKey());
#endif
    for (int i = 0; i < n; i++) {
        const TranspositionTable::TTEntry& ent = changes.ent[i];
        U64 key = ent.getKey();
        int type = ent.getType();
        int ply = 0;
        int depth = ent.getDepth();
        bool busy = ent.getBusy();
        int evalScore = ent.getEvalScore();
        Move sm;
        ent.getMove(sm);
        sm.setScore(ent.getScore(ply));
        tt.insert(key, sm, type, ply, depth, evalScore, busy);
    }
    return 0;
}

// ----------------------------------------------------------------------------

ClusterTTReceiver::ClusterTTReceiver(int cmdType, int peerRank, ClusterTT& ctt)
    : cmdType(cmdType), peerRank(peerRank), ctt(ctt), currBuf(&buffer[0]) {
    initBuf();
}

int
ClusterTTReceiver::applyChunk(const ChangeBatch& changes) {
    if (disabled)
        return INT_MAX;
    std::lock_guard<std::mutex> L(mutex);
    int n = changes.nEnts;
    for (int i = 0; i < n; i++) {
        const TranspositionTable::TTEntry& ent = changes.ent[i];
        if (ent.getDepth() < minDepth)
            continue;
        if (currBuf->size + 16 <= SearchConst::MAX_CLUSTER_BUF_SIZE) {
            U8* buf = &currBuf->data[currBuf->size];
            Serializer::serialize<64>(buf, ent.getKey(), ent.getData());
            currBuf->size += 16;
        } else {
            if (!full) {
                full = true;
                minDepth++;
            }
        }
    }
    return minDepth;
}

void
ClusterTTReceiver::initBuf() {
    currBuf->size = sizeof(int);
    full = false;
    Serializer::serialize<64>(&currBuf->data[0], cmdType);
}

bool
ClusterTTReceiver::sendBuffer(MPI_Request& sendReq) {
    if (nSendSlots <= 0)
        return false;

    std::lock_guard<std::mutex> L(mutex);
    Buffer* sendBuf = currBuf;
    currBuf = currBuf == &buffer[0] ? &buffer[1] : &buffer[0];
    initBuf();

    int count = sendBuf->size;
    if (count < SearchConst::MAX_CLUSTER_BUF_SIZE / 2) {
        if (minDepth > 0)
            minDepth--;
    }
    if (count == sizeof(int))
        return false;

    MPI_Isend(&sendBuf->data[0], count, MPI_BYTE, peerRank, 0, MPI_COMM_WORLD, &sendReq);
    nSendSlots--;
    return true;
}

void
ClusterTTReceiver::receiveBuffer(const U8* buf, int len) {
    int type;
    buf = Serializer::deSerialize<64>(buf, type);
    len  = (len - sizeof(int)) / 16;
    for (int i = 0; i < len; i++) {
        U64 key, data;
        buf = Serializer::deSerialize<64>(buf, key, data);
        ctt.insert(TranspositionTable::TTEntry(key, data));
    }
    ctt.flush();
}

void
ClusterTTReceiver::ttAck(int nAcks) {
    nSendSlots += nAcks;
}

#endif
