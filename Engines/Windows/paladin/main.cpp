#include "chess.h"

int main()
{
    // printf("\nAnkan's chess engine\n");

    UciInterface::ProcessCommands();

    return 0;
}

// ----------------------------------------------- Game Class ---------------------------------------------------//

// static variable definations
HexaBitBoardPosition Game::pos;
int Game::searchTime;
int Game::searchTimeLimit;

int Game::maxSearchDepth;

CMove Game::bestMove;
CMove Game::pv[MAX_GAME_LENGTH];
int   Game::pvLen;

uint64 Game::nodes;

#if GATHER_STATS == 1
uint32 Game::totalSearched;
uint32 Game::nonTTSearched;
uint32 Game::nonCaptureSearched;
uint32 Game::nonKillersSearched;
#endif


uint64 Game::posHashes[MAX_GAME_LENGTH];
int Game::plyNo;
uint8 Game::irreversibleMoveRefCount;

CMove Game::killers[MAX_GAME_LENGTH][MAX_KILLERS];

#if HISTORY_PER_PIECE == 1
uint32 Game::historyScore[2][6][64][64];
uint32 Game::butterflyScore[2][6][64][64];
#else
uint32 Game::historyScore[2][64][64];
uint32 Game::butterflyScore[2][64][64];
#endif

Timer Game::timer;


volatile bool Game::searching;

void Game::Reset()
{
    // initialize variables, etc
    memset(&pos, 0, sizeof(pos));
    memset(&bestMove, 0, sizeof(bestMove));
    memset(posHashes, 0, sizeof(posHashes));
    memset(killers, 0, sizeof(killers));
    memset(historyScore, 0, sizeof(historyScore));
    memset(butterflyScore, 0, sizeof(butterflyScore));

    searchTime = 0;
    searchTimeLimit = 0;
    maxSearchDepth = MAX_SEARCH_LENGTH;
    plyNo = 0;
    irreversibleMoveRefCount = 0;
}

void Game::SetTimeControls(int wtime, int btime, int movestogo, int winc, int binc, int searchTimeExact)
{
    int chance = pos.chance;

    if (searchTimeExact)
    {
        searchTime = searchTimeExact;
        searchTimeLimit = searchTimeExact;
    }
    else
    {
        int x;
        int inc;
        if (chance == WHITE)
        {
            x = wtime;
            inc = winc;
        }
        else
        {
            x = btime;
            inc = binc;
        }

        if (movestogo == 0)
        {
            movestogo = 40;
        }
        searchTime = x / (movestogo) + inc;

        // set search time limit as 2X of time normally allocated - but limited to actual time remaining
        searchTimeLimit = 2 * searchTime;

        if (searchTimeLimit >= (x + inc))
        {
            searchTimeLimit = x + inc - 1;
        }

    }
}

void Game::StartSearch()
{
    searching = true;
    nodes = 0;

#if GATHER_STATS == 1
    totalSearched = 0;
    nonTTSearched = 0;
    nonCaptureSearched = 0;
    nonKillersSearched = 0;
#endif

    timer.start();

    for (int depth = 1; depth < maxSearchDepth; depth++)
    {

        int16 eval = 0;
        try
        {
            if (pos.chance == WHITE)
                eval = alphabetaRoot<WHITE>(&pos, depth, plyNo + 1);
            else
                eval = alphabetaRoot<BLACK>(&pos, depth, plyNo + 1);
        }
        catch (std::exception exp)
        {
            break;
        }

        GetPVFromTT(&pos);

        uint64 timeElapsed = timer.stop();
        uint64 nps = nodes * 1000;
        if (timeElapsed)
        {
            nps /= timeElapsed;    // time is in ms
        }

        bool foundMate = false; // don't waste anymore time if we already found a mate
        if (abs(eval) >= MATE_SCORE_BASE)
        {
            foundMate = true;
        }

        // print mate score correctly
        if (abs(eval) >= MATE_SCORE_BASE/2)
        {
            int16 mateDepth = abs(eval) - MATE_SCORE_BASE;

            // convert the depth to be relative to current position (distance from mate)
            mateDepth = (depth - mateDepth);

            if (eval < 0)
                mateDepth = -mateDepth;

            // mateDepth is in plies -> convert it into moves
            mateDepth /= 2;

            printf("info depth %d score mate %d nodes %llu time %llu nps %llu pv ", depth, mateDepth, nodes, timeElapsed, nps);
        }
        else
        {
            printf("info depth %d score cp %d nodes %llu time %llu nps %llu pv ", depth, (int)eval, nodes, timeElapsed, nps);
        }

        // display the PV (TODO: currently the PV is wrong after second move)
        for (int i = 0; i < pvLen; i++)
        {
            Utils::displayCompactMove(pv[i]);
        }
        printf("\n");

#if GATHER_STATS == 1
        printf("total: %d, needsMoveGen: %d, needsNonCaptures: %d, needsNonKillers: %d\n", totalSearched, nonTTSearched, nonCaptureSearched, nonKillersSearched);
#endif

        fflush(stdout);

        // TODO: better time management
        if (foundMate || (timeElapsed > (searchTime / 1.3f)))
        {
            break;
        }
    }

    searching = false;
}


// super fast optimized perft routine
// one of the fastest w/o using hash tables
// on 4790k, start pos ~620 million nps, pos2 (of cpw): 1.08 billion nps
uint64 Game::perft(HexaBitBoardPosition *pos, int depth)
{
    HexaBitBoardPosition newPositions[MAX_MOVES];
    int nMoves = 0;

    if (depth == 1)
    {
        // optimization 1: bulk counting
        nMoves = BitBoardUtils::CountMoves(pos);
        return (uint64) nMoves;
    }

    // optimization 2: directly generate boards instead of first generating moves and then boards
    nMoves = BitBoardUtils::GenerateBoards(pos, newPositions);
    uint64 count = 0;

    for (int i = 0; i < nMoves; i++)
    {
        uint64 childPerft = perft(&newPositions[i], depth - 1);
        count += childPerft;
    }

    return count;
}


// tester perft routine
// check if ordered (MVV-LVA) move gen is working

// MVV-LVA move generation is 2.5X slower than regular move gen
// regular without bulk counting: start pos: 243 million nps, pos2: 367 million nps
// MVV-LVA without bulk counting: start pos:  96 million nps, pos2: 150 million nps

// also test if generateMovesCausingCheck is working

// TODO: perft 5 of pos3 of cpw is wrong! 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -
// The bug happens if we split perft into captures and non-captures. No such problem with plain old perft

template<uint8 chance>
uint64 Game::perft_test(HexaBitBoardPosition *pos, int depth)
{
    CMove genMoves[MAX_MOVES];
    int nMoves = 0;
    /*
    if (depth == 1)
    {
        nMoves = BitBoardUtils::countMoves<chance>(pos);
        return (uint64)nMoves;
    }
    */

    ExpandedBitBoard bb = BitBoardUtils::ExpandBitBoard<chance>(pos);
    bool inCheck = !!(bb.threatened & bb.myKing);

    if (inCheck)
    {
        nMoves = BitBoardUtils::generateMovesOutOfCheck<chance>(&bb, genMoves);
    }
    else
    {
        // captures
        nMoves = BitBoardUtils::generateCaptures<chance>(&bb, genMoves);

        // checks
        int checkCount = BitBoardUtils::generateMovesCausingCheck<chance>(&bb, &genMoves[nMoves]);
        nMoves += checkCount;

        // rest
        CMove tempMoves[MAX_MOVES];
        int allNonCaptures = BitBoardUtils::generateNonCaptures<chance>(&bb, tempMoves);
        // filter out checks
        int checks = 0;
        for (int i = 0; i < allNonCaptures; i++)
        {
            HexaBitBoardPosition newPos = *pos;
            uint64 hash = 0;
            BitBoardUtils::makeMove<chance>(&newPos, hash, tempMoves[i]);
            if (!BitBoardUtils::IsInCheck(&newPos))
            {
                genMoves[nMoves++] = tempMoves[i];
            }
            else
            {
                checks++;
            }
        }

        if (checkCount != checks)
        {
            printf("\nFound mismatch in check count, found %d, actual %d\n", checkCount, checks);
            Utils::dispBoard(pos);
        }

    }

    if (depth == 1)
        return (uint64) nMoves;

    uint64 count = 0;

    for (int i = 0; i < nMoves; i++)
    {
        HexaBitBoardPosition newPos = *pos;
        uint64 hash = 0;
        BitBoardUtils::makeMove<chance>(&newPos, hash, genMoves[i]);

        uint64 childPerft = perft_test<!chance>(&newPos, depth - 1);
        count += childPerft;
    }

    return count;
}



uint64 Game::Perft(int depth)
{
    //return perft(&pos, depth);
    
    
    if (pos.chance == WHITE)
        return perft_test<WHITE>(&pos, depth);
    else
        return perft_test<BLACK>(&pos, depth);
   
}
