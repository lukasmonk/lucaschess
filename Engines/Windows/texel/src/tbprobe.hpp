/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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
 * tbprobe.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#ifndef TBPROBE_HPP_
#define TBPROBE_HPP_

#include "transpositionTable.hpp"
#include "moveGen.hpp"

#include <string>


class Position;

/**
 * Handle tablebase probing.
 */
class TBProbe {
    friend class TBTest;
public:
    /** Initialize tablebases. */
    static void initialize(const std::string& gtbPath, int cacheMB,
                           const std::string& rtbPath);

    /** Return true if GTB or RTB probing is enabled. */
    static bool tbEnabled();

    /** Probe one or more tablebases to get an exact score or a usable bound.
     * @param pos  The position to probe. The position can be temporarily modified
     *             but is restored to original state before function returns.
     */
    static bool tbProbe(Position& pos, int ply, int alpha, int beta,
                        TranspositionTable::TTEntry& ent);

    /** If some TB files are missing, it may be necessary to only search a subset
     * of the root moves in order to make progress. This might happen for example
     * in KPK if the KQK table is missing and search is not able to see the mate
     * after promoting the pawn.
     * @param pos           The root position.
     * @param legalMoves    The set of legal root moves.
     * @param movesToSearch The moves to search.
     * @return True if a subset should be searched, false to search all moves.
     */
    static bool getSearchMoves(Position& pos, const MoveList& legalMoves,
                               std::vector<Move>& movesToSearch);

    /** Enhance PV with DTM information from gaviota tablebases. */
    static void extendPV(const Position& rootPos, std::vector<Move>& pv);

    /** Probe gaviota DTM tablebases.
     * @param pos  The position to probe. The position can be temporarily modified
     *             but is restored to original state before function returns.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     * @return True if pos was found in the tablebases.
     */
    static bool gtbProbeDTM(Position& pos, int ply, int& score);

    /**
     * Probe gaviota WDL tablebases.
     * @param pos  The position to probe. The position can be temporarily modified
     *             but is restored to original state before function returns.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     *              The returned score is either 0 or a mate bound.
     */
    static bool gtbProbeWDL(Position& pos, int ply, int& score);

    /**
     * Probe syzygy DTZ tablebases.
     * @param pos  The position to probe. The position can be temporarily modified
     *             but is restored to original state before function returns.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     *              The returned score is either 0 or a mate bound. The bound
     *              is computed by considering the DTZ value and the maximum number
     *              of zeroing moves before mate.
     */
    static bool rtbProbeDTZ(Position& pos, int ply, int& score);

    /**
     * Probe syzygy WDL tablebases.
     * @param pos  The position to probe. The position can be temporarily modified
     *             but is restored to original state before function returns.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     *              The returned score is either 0 or a mate bound.
     */
    static bool rtbProbeWDL(Position& pos, int ply, int& score);

private:
    /** Initialize */
    static void gtbInitialize(const std::string& path, int cacheMB, int wdlFraction);

    static void initWDLBounds();
    static int getMaxDTZ(int matId);
    static int getMaxSubMate(const Position& pos);
    static int getMaxSubMate(std::vector<int>& pieces, int pawnMoves);
    static void initMaxDTM();
    static void initMaxDTZ();

    struct GtbProbeData {
        unsigned int stm, epsq, castles;
        static const int MAXLEN = 17;
        unsigned int  wSq[MAXLEN];
        unsigned int  bSq[MAXLEN];
        unsigned char wP[MAXLEN];
        unsigned char bP[MAXLEN];
        int materialId;
    };

    /** Convert position to GTB probe format. */
    static void getGTBProbeData(const Position& pos, GtbProbeData& gtbData);

    static bool gtbProbeDTM(const GtbProbeData& gtbData, int ply, int& score);

    static bool gtbProbeWDL(const GtbProbeData& gtbData, int ply, int& score);
};


#endif /* TBPROBE_HPP_ */
