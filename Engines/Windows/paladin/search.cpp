#include "chess.h"

// good page on quiescent-search
// http://web.archive.org/web/20040427014440/brucemo.com/compchess/programming/quiescent.htm#MVVLVA

// Quiescence search
template<uint8 chance>
int16 Game::q_search(HexaBitBoardPosition *pos, uint64 hash, int depth, int16 alpha, int16 beta, int curPly)
{

    nodes++;    // node count is the no. of nodes on which Evaluation function is called

    bool improvedAlpha = false;

    int16 evalFromTT;
    uint8 scoreType;
    if (TranspositionTable::lookup_q(hash, &evalFromTT, &scoreType))
    {
        if (scoreType == SCORE_EXACT)
        {
            return evalFromTT;
        }
        if (scoreType == SCORE_GE && evalFromTT >= beta)
        {
            return evalFromTT;
        }
    }

    int16 currentMax = -INF;

    // check current position
    int16 stand_pat = BitBoardUtils::Evaluate(pos);
    
    if (stand_pat >= beta)
    {
        TranspositionTable::update_q(hash, stand_pat, SCORE_GE);
        return stand_pat;
    }

#if Q_SEARCH_CHECK_EXTENSIONS == 1
    // to detect infinite check->check evasion loops
    posHashes[curPly] = hash;
#endif

    currentMax = stand_pat;
    if (currentMax > alpha)
    {
        alpha = currentMax;
        improvedAlpha = true;
    }


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    bool inCheck = !!(bb.threatened & bb.myKing);

    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);

        if (nMoves == 0) // mate!
        {
            TranspositionTable::update_q(hash, -MATE_SCORE_BASE, SCORE_EXACT);   // TODO: this might cause problems ?
            return -(MATE_SCORE_BASE + depth);
        }
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

        // sorting captures using SEE doesn't seem to benefit at all :-(
    }

    for (int i = 0; i < nMoves; i++)
    {
#if USE_Q_SEARCH_SEE_PRUNING == 1
        if ((!inCheck) && BitBoardUtils::EvaluateSEE<chance>(pos, newMoves[i]) <= 0)
            continue;
#endif

        HexaBitBoardPosition newPos = *pos;
        uint64 newhash = hash;

        BitBoardUtils::MakeMove(&newPos, newhash, newMoves[i]);

        int16 curScore = -q_search<!chance>(&newPos, newhash, depth - 1, -beta, -alpha, curPly + 1);
        if (curScore >= beta)
        {
            TranspositionTable::update_q(hash, curScore, SCORE_GE);
            return curScore;
        }

        if (curScore > currentMax)
        {
            currentMax = curScore;
            if (currentMax > alpha)
            {
                alpha = currentMax;
                improvedAlpha = true;
            }
        }
    }

#if Q_SEARCH_CHECK_EXTENSIONS == 1
    // check extensions: try moves that would put opponent side in check
    if (!inCheck)
    {
        // avoid stack overflow due to repeated checks and check evasions
        // TODO: no need to search so many plies - reset the counter whenever a capture is made
        for (int i = 0; i < curPly; i++)
        {
            if (posHashes[i] == hash)
            {
                return 0;   // draw by repetition
            }
        }

        nMoves = BitBoardUtils::generateMovesCausingCheck<chance>(&bb, newMoves);

        for (int i = 0; i < nMoves; i++)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newhash = hash;

            BitBoardUtils::MakeMove(&newPos, newhash, newMoves[i]);

            int16 curScore = -q_search<!chance>(&newPos, newhash, depth - 1, -beta, -alpha, curPly + 1);
            if (curScore >= beta)
            {
                TranspositionTable::update_q(hash, curScore, SCORE_GE);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }
            }
        }
    }
#endif

    if (improvedAlpha)
    {
        TranspositionTable::update_q(hash, currentMax, SCORE_EXACT);
    }
    return currentMax;
}

// update history
// TODO: do we want to clear history tables after every move actually made ?
void Game::UpdateHistory(HexaBitBoardPosition *pos, CMove move, int depth, uint8 chance, bool betaCutoff)
{
#if USE_HISTORY_HEURISTIC == 1

    // adding history data for all depths seems to help a tiny bit (reduces nodes searched.. but increases time slightly)
    //if (depth < HISTORY_SORT_MIN_DEPTH)
    //    return;

#if HISTORY_PER_PIECE == 1
    uint8 piece = BitBoardUtils::getPieceAtSquare(pos, BIT(move.getFrom())) - 1;
    #define ARRAY_DIM [piece]
#else
    #define ARRAY_DIM
#endif

    // update history table
    if (betaCutoff)
    {
        historyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] += depth * depth;
        if (historyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] > INT_MAX)
        {
            historyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] /= 2;
            butterflyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] /= 2;
        }
    }
    else
    {
        // update butterfly table
        butterflyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] += depth * depth;
        if (butterflyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] > INT_MAX)
        {
            butterflyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] /= 2;
            historyScore[chance]ARRAY_DIM[move.getFrom()][move.getTo()] /= 2;
        }
    }
#endif
}

// sort quiet moves based on history heuristic
void Game::SortMovesHistory(HexaBitBoardPosition *pos, CMove* moves, int nMoves, uint8 chance)
{
    float scores[MAX_MOVES];
    for (int i = 0; i < nMoves; i++)
    {
        if (!moves[i].isValid())
        {
            scores[i] = 0.0f;
            continue;
        }

#if HISTORY_PER_PIECE == 1
        uint8 piece = BitBoardUtils::getPieceAtSquare(pos, BIT(moves[i].getFrom())) - 1;
        #define ARRAY_DIM [piece]
#else
        #define ARRAY_DIM
#endif

        if (butterflyScore[chance]ARRAY_DIM[moves[i].getFrom()][moves[i].getTo()] == 0)
        {
            scores[i] = (float) historyScore[chance]ARRAY_DIM[moves[i].getFrom()][moves[i].getTo()];
        }
        else
        {
            scores[i] = ((float)   historyScore[chance]ARRAY_DIM[moves[i].getFrom()][moves[i].getTo()]) /
                                 butterflyScore[chance]ARRAY_DIM[moves[i].getFrom()][moves[i].getTo()] ;
        }
    }

    // sort move list based on score 
    // use insertion sort - might be too slow 
    // TODO: (we better avoid sorting and try picking best from the list when trying out moves (faster if we have a beta cutoff soon...)
    for (int i = 1; i < nMoves; i++)
    {
        int j = i;
        float scoreX = scores[j];
        CMove moveX = moves[j];
        while (j > 0 && scores[j - 1] < scoreX)
        {
            scores[j] = scores[j - 1];
            moves[j] = moves[j - 1];
            j--;
        }
        scores[j] = scoreX;
        moves[j] = moveX;
    }
}

// returns the index of first non-winning capture (or the no. of winning captures)
template<uint8 chance>
int16 Game::SortCapturesSEE(HexaBitBoardPosition *pos, CMove* captures, int nMoves)
{
    int16 scores[MAX_MOVES];

    // 1. evaluate all caputres using SEE
    for (int i = 0; i < nMoves; i++)
    {
        scores[i] = BitBoardUtils::EvaluateSEE<chance>(pos, captures[i]);
    }

    // sort captures list based on score 
    // use insertion sort as data set is expected to be very small (taken from wikipedia)
    for (int i = 1; i < nMoves; i++)
    {
        int j = i;
        int scoreX = scores[j];
        CMove moveX = captures[j];
        while (j > 0 && scores[j - 1] < scoreX)
        {
            scores[j] = scores[j - 1];
            captures[j] = captures[j - 1];
            j--;
        }
        scores[j] = scoreX;
        captures[j] = moveX;
    }

    // find the index of first non-winning capture
    // we can use binary search or merge this with the outer loop of the above block - but the complexity is not worth it
    int winningCaptures = 0;
    while (winningCaptures < nMoves &&
           scores[winningCaptures] > 0)
    {
        winningCaptures++;
    }
    
    return winningCaptures;
}


// no of plies to reduce for LMR
// no - longer used - we always reduce by 1 ply
static int getLMRReduction(int depth, bool isPVNode, int movesSearched)
{
    if (isPVNode)
        return 1;

    if (movesSearched <= 8)
        return 1;

    if (depth >= 6)
        return 2;

    return 1;
}

// adjust mate score returned to the ply where we extended the depth
static int16 adjustScoreForExtension(int16 score, bool extended)
{
    if (!extended)
        return score;

    if (abs(score) >= MATE_SCORE_BASE / 2)
    {
        if (score < 0)
            score++;
        else
            score--;
    }

    return score;
}

// negamax forumlation of alpha-beta search
template<uint8 chance>
int16 Game::alphabeta(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool allowNullMove, CMove lastMove)
{
    // check for timeout
    if (depth > 3)
    {
        uint64 timeElapsed = timer.stop();
        if (timeElapsed > (searchTimeLimit))
            throw std::exception();
    }

    // expand bitboard structure (TODO: come up with something that doesn't need expanding ?)
    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);

    // detect basic draws (insufficient material)
    if (BitBoardUtils::isDrawn(bb))
    {
        return 0;
    }

    bool inCheck = !!(bb.threatened & bb.myKing);

    bool extendedDepth = false;

#if USE_CHECK_EXTENSIONS == 1
    if (inCheck)
    {
        depth++;
        extendedDepth = true;
    }
#endif

#if USE_PROMOTION_EXTENSION == 1
    if (extendedDepth == false)
    {
        uint64 dst = BIT(lastMove.getTo());
        if ((dst & (RANK2 | RANK7)) && BitBoardUtils::getPieceAtSquare(pos, dst) == PAWN)
        {
            depth++;
            extendedDepth = true;
        }
    }
#endif

    if (depth == 0)
    {
        int16 qSearchVal = q_search<chance>(pos, hash, depth, alpha, beta, curPly);
        return adjustScoreForExtension(qSearchVal, extendedDepth);
    }

    // detect draw by repetition
    posHashes[curPly] = hash;
    for (int i = 0; i < curPly; i++)
    {
        if (posHashes[i] == hash)
        {
            // don't update TT for draw by repetition
            return 0;   // draw by repetition
        }
    }


#if USE_NULL_MOVE_PRUNING == 1
    // try null move
    
    // the reduction factor (no of plies we save by doing null move search first)
    int R = DEFAULT_NULL_MOVE_REDUCTION;

    // TODO: more checks to avoid dangerous positions
    if (allowNullMove &&                                                    // avoid doing null move twice in a row
        !inCheck &&                                                         // can't do null move when in check
        depth > R &&                                                        // can't do near horizon
        BitBoardUtils::countMoves<chance>(pos) >= MIN_MOVES_FOR_NULL_MOVE &&
        BitBoardUtils::countMoves<!chance>(pos) >= MIN_MOVES_FOR_NULL_MOVE) // avoid when there are very few valid moves
    {

        if (depth >= MIN_DEPTH_FOR_EXTRA_REDUCTION)
        {
            R = EXTRA_NULL_MOVE_REDUCTION;
        }

        // make null move

        // 1. reverse the chance
        pos->chance = !pos->chance;
        uint64 newHash = hash ^ BitBoardUtils::zob.chance;

        // 2. clear en-passent flag if set
        uint8 ep = pos->enPassent;
        pos->enPassent = 0;
        if (ep)
        {
            newHash ^= BitBoardUtils::zob.enPassentTarget[ep - 1];
        }

        int16 nullMoveScore = -alphabeta<!chance>(pos, newHash, depth - 1 - R, curPly + 1, -beta, -beta + 1, false, CMove(0));

        nullMoveScore = adjustScoreForExtension(nullMoveScore, extendedDepth);
        

        // undo null move
        pos->chance = !pos->chance;
        pos->enPassent = ep;

        if (nullMoveScore >= beta) return nullMoveScore;
    }
#endif    


    // lookup in the transposition table
    int hashDepth = 0;
    int16 hashScore = 0;
    uint8 scoreType = 0;
    CMove ttMove = {};
    bool foundInTT = TranspositionTable::lookup(hash, depth, &hashScore, &scoreType, &hashDepth, &ttMove);
    if (foundInTT && hashDepth >= depth)
    {
        // exact score at same or better depth => done, return TT value
        if (scoreType == SCORE_EXACT)
        {
            return hashScore;
        }

        // score at same or better depth causes beta cutoff - again return TT value
        if (scoreType == SCORE_GE && hashScore >= beta)
        {
            return hashScore;
        }

        // score causes alpha-cutoff
        if (scoreType == SCORE_LE && hashScore <= alpha)
        {
            return hashScore;
        }
    }


    // Internal iterative deepening
    // good discussion here:
    // http://www.open-aurec.com/wbforum/viewtopic.php?f=4&t=4698
    if (depth >= MIN_DEPTH_FOR_IID)
    {
        int iidDepth = depth - 1;

        // account for extensions
        if (extendedDepth)
        {
            iidDepth--;
        }

        if (hashDepth < iidDepth)
        {
            alphabeta<chance>(pos, hash, iidDepth, curPly, alpha, beta, allowNullMove, lastMove);

            // again query the TT (to get updated value)
            foundInTT = TranspositionTable::lookup(hash, depth, &hashScore, &scoreType, &hashDepth, &ttMove);
        }
    }


#if GATHER_STATS == 1
    totalSearched++;
#endif

    int16 currentMax = -INF;

    int movesSearched = 0;

    bool improvedAlpha = false;

    // check hash move first
    CMove currentBestMove = ttMove;
    if (ttMove.isValid())
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 newHash = hash;
        BitBoardUtils::MakeMove(&newPos, newHash, ttMove);

        movesSearched++;
        int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, ttMove);
        curScore = adjustScoreForExtension(curScore, extendedDepth);

        // update history tables if this was a non-capture move
        if (!(ttMove.getFlags() & CM_FLAG_CAPTURE))
        {
            UpdateHistory(pos, ttMove, depth, chance, curScore >= beta);
        }

        if (curScore >= beta)
        {
            // update killer move table if this was a non-capture move
            if (!(ttMove.getFlags() & CM_FLAG_CAPTURE))
            {
                if (killers[depth][0] != ttMove)
                {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = ttMove;
                }
            }

            TranspositionTable::update(hash, curScore, SCORE_GE, currentBestMove, depth, curPly);
            return curScore;
        }

        currentMax = curScore;
        if (currentMax > alpha)
        {
            alpha = currentMax;
            improvedAlpha = true;
        }
    }

#if GATHER_STATS == 1
    nonTTSearched++;
#endif

    // searched points to the index in newMoves list
    // but we might have tried more moves than that (e.g from TT or killers)
    int searched = 0;
    int winingCaptures = 0;

    // generate child nodes
    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);
        winingCaptures = nMoves;
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order


#if USE_SEE_MOVE_ORDERING == 1
        // sorting captures using SEE is overall a loss and sometimes even results in bigger tree! Bug?
        // searching losing captures after quiet moves results in even bigger tree! Another bug?
        if (depth >= MIN_DEPTH_FOR_SEE)
        {
            winingCaptures = SortCapturesSEE<chance>(pos, newMoves, nMoves);
        }
#endif

#if SEARCH_LOSING_CAPTURES_AFTER_KILLERS == 0
        winingCaptures = nMoves;
#endif

        // search captures first
        for (int i = 0; i < winingCaptures; i++)
        {
            if (newMoves[i] != ttMove)
            {
                HexaBitBoardPosition newPos = *pos;
                uint64 newHash = hash;
                BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

                movesSearched++;
                int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);
                curScore = adjustScoreForExtension(curScore, extendedDepth);

                if (curScore >= beta)
                {
                    TranspositionTable::update(hash, curScore, SCORE_GE, newMoves[i], depth, curPly);
                    return curScore;
                }

                if (curScore > currentMax)
                {
                    currentMax = curScore;
                    if (currentMax > alpha)
                    {
                        alpha = currentMax;
                        improvedAlpha = true;
                    }

                    currentBestMove = newMoves[i];
                }
            }
        }
        searched = nMoves;

        nMoves += BitBoardUtils::generateNonCaptures<chance>(&bb, &newMoves[nMoves]);   // then rest of the moves
    }


#if GATHER_STATS == 1
    nonCaptureSearched++;
#endif

    // special case: Check if it's checkmate or stalemate
    if (nMoves == 0)
    {
        if (inCheck)
        {
            int16 curScore = -(MATE_SCORE_BASE + depth);
            TranspositionTable::update(hash, curScore, SCORE_EXACT, CMove(), depth, curPly);
            return curScore;
        }
        else
        {
            TranspositionTable::update(hash, 0, SCORE_EXACT, CMove(), depth, curPly);
            return 0;
        }
    }

    // try killers first
    // also filter out killers and TT move from the list 
    // TODO: no need to generate non captures if we get a cutoff on trying killer moves
    //       need a move validity checker routine
    for (int i = searched; i < nMoves; i++)
    {
        // filter TT moves
        if (newMoves[i] == ttMove)
        {
            newMoves[i] = CMove();
            continue;
        }

        bool isKiller = (killers[depth][0] == newMoves[i]) || 
                        (killers[depth][1] == newMoves[i]);

        if (isKiller)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = hash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            movesSearched++;
            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);
            curScore = adjustScoreForExtension(curScore, extendedDepth);

            UpdateHistory(pos, newMoves[i], depth, chance, curScore >= beta);

            if (curScore >= beta)
            {
                // increase priority of this killer
                if (killers[depth][0] != newMoves[i])
                {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = newMoves[i];
                }

                TranspositionTable::update(hash, curScore, SCORE_GE, newMoves[i], depth, curPly);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }

                currentBestMove = newMoves[i];
            }

            // filter out killers
            newMoves[i] = CMove();
        }
    }

#if GATHER_STATS == 1
    nonKillersSearched++;
#endif

#if SEARCH_LOSING_CAPTURES_AFTER_KILLERS == 1
    // search losing captures
    for (int i = winingCaptures; i < searched; i++)
    {
        if (newMoves[i] != ttMove)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = hash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            movesSearched++;
            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);
            curScore = adjustScoreForExtension(curScore, extendedDepth);

            if (curScore >= beta)
            {
                TranspositionTable::update(hash, curScore, SCORE_GE, newMoves[i], depth, curPly);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }

                currentBestMove = newMoves[i];
            }
        }
    }
#endif

    // used for LMR
#if USE_LATE_MOVE_REDUCTION == 1
    int16 standpat = 0;
    if (depth >= LMR_MIN_DEPTH)
    {
        standpat = BitBoardUtils::Evaluate(pos);
    }
#endif

#if USE_HISTORY_HEURISTIC == 1
    // sort non-captures based on history heuristic
    if (depth >= HISTORY_SORT_MIN_DEPTH)
    {
        SortMovesHistory(pos, &newMoves[searched], nMoves - searched, chance);
    }
#endif

    // try remaining moves (non capture, non TT and non-killer)
    // searched == no. of captures when not in check, and 0 when in check
    for (int i = searched; i < nMoves; i++)
    {
        if (newMoves[i].isValid())
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = hash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            bool needFullDepthSearch = true;
            int16 curScore = 0;

#if USE_LATE_MOVE_REDUCTION == 1
            // try late move reduction
            // see http://www.glaurungchess.com/lmr.html for a good introduction to LMR

            if (depth >= LMR_MIN_DEPTH &&                                   // we are at sufficient depth
                movesSearched >= LMR_FULL_DEPTH_MOVES &&                    // sufficient no. of moves have been already searched at full depth
                movesSearched - searched >= LMR_FULL_DEPTH_QUEIT_MOVES &&   // sufficient no. of quite moves have been searched at full depth
                !inCheck &&                                                 // not in check
              //!improvedAlpha &&                                           // this is not a PV node (makes engine significantly weaker!)
                standpat - LMR_EVAL_THRESHOLD < alpha &&                    // static eval at the node is less than best move found (again doesn't seem to help)
              //!(BIT(newMoves[i].getFrom()) & bb.pawns) &&                 // don't reduce pawn pushes
                !BitBoardUtils::IsInCheck(&newPos)                          // the move doesn't cause a check to opponent side
                )
            {
                // search with reduced depth (also notice null-window - i.e, beta = alpha+1 as we are only interested in checking if the returned value is > alpha)
                curScore = -alphabeta<!chance>(&newPos, newHash, depth - 2 /*- getLMRReduction(depth, improvedAlpha, movesSearched)*/, curPly + 1, -(alpha + 1), -alpha, true, newMoves[i]);
                if (curScore <= currentMax)
                {
                    needFullDepthSearch = false;
                }
            }
#endif

            if (needFullDepthSearch)
            {
                curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);
            }
            curScore = adjustScoreForExtension(curScore, extendedDepth);

            movesSearched++;

            UpdateHistory(pos, newMoves[i], depth, chance, curScore >= beta);

            if (curScore >= beta)
            {
                // update killer table
                killers[depth][1] = killers[depth][0];
                killers[depth][0] = newMoves[i];

                TranspositionTable::update(hash, curScore, SCORE_GE, newMoves[i], depth, curPly);
                return curScore;
            }

            if (curScore > currentMax)
            {
                currentMax = curScore;
                if (currentMax > alpha)
                {
                    alpha = currentMax;
                    improvedAlpha = true;
                }

                currentBestMove = newMoves[i];
            }
        }
    }


    // default node type is ALL node and the score returned is a upper bound on the score of the node
    if (improvedAlpha)
    {
        scoreType = SCORE_EXACT; 
    }
    else
    {
        // ALL node
        scoreType = SCORE_LE;
    }
    TranspositionTable::update(hash, currentMax, scoreType, currentBestMove, depth, curPly);

    return currentMax;
}

void Game::GetPVFromTT(HexaBitBoardPosition *pos)
{
    HexaBitBoardPosition nextPos = *pos;

    int depth = 0;
    while (depth < MAX_GAME_LENGTH)
    {
        uint64 posHash = BitBoardUtils::ComputeZobristKey(&nextPos);

        CMove bestMove = {};
        int16 score;
        uint8 type;
        int hashDepth;
        bool foundInTT = TranspositionTable::lookup(posHash, 0, &score, &type, &hashDepth, &bestMove);
        if (foundInTT && bestMove.isValid())
        {
            pv[depth++] = bestMove;
            BitBoardUtils::MakeMove(&nextPos, posHash, bestMove);
        }
        else
        {
            break;
        }
    }

    pvLen = depth;
}

// root of alpha-beta search
template<uint8 chance>
int16 Game::alphabetaRoot(HexaBitBoardPosition *pos, int depth, int curPly)
{
    int16 alpha = -INF, beta = INF;
    uint64 posHash = BitBoardUtils::ComputeZobristKey(pos);

    // used to detect draw by repetition
    posHashes[curPly] = posHash;

    // lookup in the transposition table
    int hashDepth = 0;
    int16 hashScore = 0;
    uint8 scoreType = 0;
    CMove ttMove = {};
    bool foundInTT = TranspositionTable::lookup(posHash, depth, &hashScore, &scoreType, &hashDepth, &ttMove);

    CMove currentBestMove = ttMove;
    if (ttMove.isValid())
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 newHash = posHash;
        BitBoardUtils::MakeMove(&newPos, newHash, ttMove);

        int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, ttMove);

        if (curScore > alpha)
        {
            alpha = curScore;
        }
    }


    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    int searched = 0;
    bool inCheck = !!(bb.threatened & bb.myKing);

    // generate child nodes
    CMove newMoves[MAX_MOVES];
    int nMoves;

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, newMoves);
    }
    else
    {
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, newMoves);                // generate captures in MVV-LVA order

#if USE_SEE_MOVE_ORDERING == 1
        // sorting captures using SEE at root node always results in bigger tree (for pos 2 of cpw)! Bug?
        SortCapturesSEE<chance>(pos, newMoves, nMoves);
#endif

        // search (winning) captures first
        for (int i = 0; i < nMoves; i++)
        {
            if (newMoves[i] != ttMove)
            {
                HexaBitBoardPosition newPos = *pos;
                uint64 newHash = posHash;
                BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

                int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);

                if (curScore > alpha)
                {
                    alpha = curScore;
                    currentBestMove = newMoves[i];
                }

                // check if we are out of time.. and exit the search if so
                uint64 timeElapsed = timer.stop();
                if (timeElapsed > (searchTime / 1.01f))
                {
                    TranspositionTable::update(posHash, alpha, SCORE_GE, currentBestMove, depth, curPly);
                    bestMove = currentBestMove;
                    return alpha;
                }
            }
        }
        searched = nMoves;

        nMoves += BitBoardUtils::generateNonCaptures<chance>(&bb, &newMoves[nMoves]);   // then rest of the moves
    }


    for (int i = searched; i < nMoves; i++)
    {
        if (newMoves[i] != ttMove)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 newHash = posHash;
            BitBoardUtils::MakeMove(&newPos, newHash, newMoves[i]);

            int16 curScore = -alphabeta<!chance>(&newPos, newHash, depth - 1, curPly + 1, -beta, -alpha, true, newMoves[i]);

            if (curScore > alpha)
            {
                alpha = curScore;
                currentBestMove = newMoves[i];
            }

            // check if we are out of time.. and exit the search if so
            uint64 timeElapsed = timer.stop();
            if (timeElapsed > (searchTime / 1.01f))
            {
                TranspositionTable::update(posHash, alpha, SCORE_GE, currentBestMove, depth, curPly);
                bestMove = currentBestMove;
                return alpha;
            }

        }
    }

    TranspositionTable::update(posHash, alpha, SCORE_EXACT, currentBestMove, depth, curPly);

    bestMove = currentBestMove;

    return alpha;
}



template int16 Game::alphabeta<WHITE>(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool tryNullMove, CMove lastMove);
template int16 Game::alphabeta<BLACK>(HexaBitBoardPosition *pos, uint64 hash, int depth, int curPly, int16 alpha, int16 beta, bool tryNullMove, CMove lastMove);


template int16 Game::alphabetaRoot<WHITE>(HexaBitBoardPosition *pos, int depth, int curPly);
template int16 Game::alphabetaRoot<BLACK>(HexaBitBoardPosition *pos, int depth, int curPly);

template int16 Game::q_search<WHITE>(HexaBitBoardPosition *pos, uint64 hash, int depth, int16 alpha, int16 beta, int curPly);
template int16 Game::q_search<BLACK>(HexaBitBoardPosition *pos, uint64 hash, int depth, int16 alpha, int16 beta, int curPly);




// transposition table related stuff
#if USE_DUAL_SLOT_TT == 1
DualTTEntry* TranspositionTable::TT;         // the transposition table
#else
TTEntry* TranspositionTable::TT;         // the transposition table
#endif
uint64   TranspositionTable::size;       // in elements
uint64   TranspositionTable::indexBits;  // bits of hash key used for the index part in hash table (size-1)
uint64   TranspositionTable::hashBits;   // bits of hash key used for the hash part in hash table (size-1)

uint64* TranspositionTable::qTT;         // transposition table for q-search


void  TranspositionTable::init(int byteSize)
{
#if USE_DUAL_SLOT_TT == 1
    size  = byteSize / sizeof(DualTTEntry);
    TT = (DualTTEntry *) malloc(byteSize);
#else
    size  = byteSize / sizeof(TTEntry);
    TT = (TTEntry *) malloc(byteSize);
#endif

    indexBits = size - 1;
    hashBits  = ALLSET ^ indexBits;

    qTT = (uint64 *) malloc(sizeof(uint64) * Q_TT_ELEMENTS);

    reset();
}

void  TranspositionTable::destroy()
{
    free (TT);
    free(qTT);
}

void  TranspositionTable::reset()
{
#if USE_DUAL_SLOT_TT == 1
    memset(TT, 0, size * sizeof(DualTTEntry));
#else
    memset(TT, 0, size * sizeof(TTEntry));
#endif
    memset(qTT, 0, Q_TT_ELEMENTS * sizeof(uint64));
}

bool TranspositionTable::lookup(uint64 hash, int searchDepth, int16 *score, uint8 *scoreType, int *foundDepth, CMove *bestMove)
{
#if USE_DUAL_SLOT_TT == 1

    DualTTEntry *entry = &TT[hash & indexBits];

    bool found = false;

    // check deepest first and then most recent
    if ((entry->deepest.hashKey & hashBits) == (hash & hashBits))
    {
        *score = entry->scoreDeepest;
        *scoreType = entry->scoreTypeDeepest;
        *foundDepth = entry->depthDeepest;
        *bestMove = CMove(entry->deepest.bestMove);
        found = true;
    }
    else if ((entry->mostRecent.hashKey & hashBits) == (hash & hashBits))
    {
        *score = entry->scoreMostRecent;
        *scoreType = entry->scoreTypeMostRecent;
        *foundDepth = entry->depthMostRecent;
        *bestMove = CMove(entry->mostRecent.bestMove);
        found = true;
    }


    if (found)
    {
        // adjust mate score
        if (abs(*score) >= MATE_SCORE_BASE / 2)
        {
            if ((*score) < 0)
                *score = (*score) - searchDepth;
            else
                *score = (*score) + searchDepth;
        }
    }

    return found;
#else
    TTEntry entry = TT[hash & indexBits];
    if (entry.hashKey == hash)
    {
        // convert distance to mate to absolute score for mates
        if (abs(entry.score) >= MATE_SCORE_BASE/2)
        {
            if (entry.score < 0)
                entry.score = entry.score - searchDepth;
            else
                entry.score = entry.score + searchDepth;
        }

        *score      = entry.score;
        *scoreType  = entry.scoreType;
        *foundDepth = entry.depth;
        *bestMove   = CMove(entry.bestMove);

        return true;
    }
    else
    {
        return false;
    }
#endif
}

void TranspositionTable::update(uint64 hash, int16 score, uint8 scoreType, CMove bestMove, int depth, int age)
{
    // hack: fix mate score. Always store mate as distance to mate from the current position
    // normally we return -(MATE_SCORE_BASE + depth);
    if (abs(score) >= MATE_SCORE_BASE/2)
    {
        if (score < 0)
            score = score + depth;
        else
            score = score - depth;
    }

#if USE_DUAL_SLOT_TT == 1
    DualTTEntry *entry = &TT[hash & indexBits];

    // try putting it in deepest slot if possible
    if (depth >= entry->depthDeepest ||                                     // either entry is deeper than what is stored
        abs(Game::GetIrReversibleRefCount() - entry->ageDeepest) >= 2)      // or what is stored is too old (2 more more ir-reversible moves made)
    {
        entry->ageDeepest = Game::GetIrReversibleRefCount();
        entry->depthDeepest = depth;
        entry->scoreDeepest = score;
        entry->scoreTypeDeepest = scoreType;
        entry->deepest.hashKey  = hash;
        entry->deepest.bestMove = bestMove.getVal();
    }
    else
    {
        // put it in most recent slot
        entry->depthMostRecent = depth;
        entry->scoreMostRecent = score;
        entry->scoreTypeMostRecent = scoreType;
        entry->mostRecent.hashKey = hash;
        entry->mostRecent.bestMove = bestMove.getVal();
    }
#else

    // TODO: better replacement strategy
    TTEntry *oldentry = &TT[hash & indexBits];
    if (age - oldentry->age > 32 ||
        depth >= oldentry->depth)
    {
        TTEntry entry;
        entry.age = age;
        entry.bestMove = bestMove.getVal();
        entry.depth = depth;
        entry.hashKey = hash;
        entry.score = score;
        entry.scoreType = scoreType;

        TT[hash & indexBits] = entry;
    }
#endif
}



bool TranspositionTable::lookup_q(uint64 hash, int16 *eval, uint8 *scoreType)
{
#if USE_Q_TT == 1
    uint64 fromTT = qTT[hash & Q_TT_INDEX_BITS];

    if ((fromTT & Q_TT_HASH_BITS) == (hash & Q_TT_HASH_BITS))
    {
        uint32 retVal = fromTT & Q_TT_INDEX_BITS;
        *eval = (retVal & 0xFFFF);
        *scoreType = ((retVal >> 16) & 0x3);

        return true;
    }
#endif
    return false;
}

void  TranspositionTable::update_q(uint64 hash, int16 eval, uint8 scoreType)
{
#if USE_Q_TT == 1
    uint32 storedval = (eval & 0xFFFF) | ((scoreType << 16) & 0x30000);
    uint64 toTT = (hash & Q_TT_HASH_BITS) | (storedval & Q_TT_INDEX_BITS);
    qTT[hash & Q_TT_INDEX_BITS] = toTT;
#endif
}
