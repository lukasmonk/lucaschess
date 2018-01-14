#include "chess.h"
// UCI Interfacing routines

void UciInterface::Search_Go(char *params) 
{
    char * str;

    // time in milliseconds remaining for white and black (for movestogo moves)
    int wtime = 0, btime = 0;
    long movestogo = 40;

    // increments (per move) for white and black
    int winc = 0, binc = 0;

    int searchTimeExact = 0;
    
    long maxDepth = 0;

    str = strstr(params, "movestogo");
    if (str) 
    {
        str += 10;
        sscanf(str, "%d", &movestogo);
    }

    str = strstr(params, "wtime");
    if (str) 
    {
        str += 6;
        sscanf(str, "%d", &wtime);
    }

    str = strstr(params, "btime");
    if (str) {
        str += 6;
        sscanf(str, "%d", &btime);
    }

    str = strstr(params, "winc");
    if (str) 
    {
        str += 5;
        sscanf(str, "%d", &winc);
    }

    str = strstr(params, "binc");
    if (str) 
    {
        str += 5;
        sscanf(str, "%d", &binc);
    }

    str = strstr(params, "depth");
    if (str) {
        str += 6;
        sscanf(str, "%d", &maxDepth);
    }

    str = strstr(params, "movetime");
    if (str) {
        str += 9;
        sscanf(str, "%d", &searchTimeExact);
    }

    str = strstr(params, "infinite");
    if (str) 
    {
        searchTimeExact = INF;
    }

    Game::SetTimeControls(wtime, btime, movestogo, winc, binc, searchTimeExact);

    
    // we shouldn't be already searching
    assert(Game::searching == false);

    if (worker_thread)
    {
        // the worker thread id gets automatically deleted when it exits?
        worker_thread = NULL;
    }
    
    // create the worker thread and return
    worker_thread = new std::thread(WorkerThreadMain);

}

void UciInterface::WorkerThreadMain()
{
    Game::StartSearch();
    printf("bestmove ");
    Utils::displayCompactMove(Game::GetBestMove());
    printf("\n");
    fflush(stdout);
}

void UciInterface::ProcessCommands() 
{
    char buffer[8192];
    char *input = NULL;

    // the main game loop
    while (1) {
        input = buffer;
        gets(input);
        if (strstr(input, "ucinewgame")) 
        {
            // new game
            Game::Reset();
            TranspositionTable::reset();
        }
        else if (strstr(input, "uci")) 
        {
            // first command to indicate uci mode

            // send back the IDs
            printf("id name Paladin 0.1\n");
            printf("id author Ankan Banerjee\n");
            fflush(stdout);
            BitBoardUtils::init();
            TranspositionTable::init();

            // send the "uciok" command
            printf("uciok\n");
            fflush(stdout);
        }
        else if (strstr(input, "isready")) 
        {
            // send back readyok command
            printf("readyok\n");
        }
        else if (strstr(input, "position")) 
        {
            BoardPosition088 temp;
            HexaBitBoardPosition pos;

            int nPlies = 0;
            int nMoves = 0;

            if (strstr(input, "startpos")) 
            {
                char startposFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
                nMoves = Utils::readFENString(startposFen, &temp) - 1; // start position
            }
            else 
            {
                input = strstr(input, "fen");
                assert(input);
                input = input + 4;	// skip the "fen "

                // read the fen string
                nMoves = Utils::readFENString(input, &temp) - 1;
            }
            Utils::board088ToHexBB(&pos, &temp);

            nPlies = 0; 
            int moveRef = 0;


            // read the moves
            input = strstr(input, "moves");
            if (input)
            {
                input += 6;	// skip the "moves "

                while ((*input) && (*input) != '\n')
                {
                    Game::SetHashForPly(nPlies, BitBoardUtils::ComputeZobristKey(&pos));    // last one is set by the search

                    CMove move;
                    int mlen = Utils::readMove(input, &pos, &move);

                    if (BitBoardUtils::IsIrReversibleMove(&pos, move))
                        moveRef++;

                    uint64 zero;
                    // make the move to update the pos in the game
                    BitBoardUtils::MakeMove(&pos, zero, move);

                    input += mlen;
                    // ignore blank spaces
                    while (*input == ' ') input++;
                    nPlies++;
                }
            }
            Game::SetPos(&pos);
            Game::SetIrReversibleRefCount(moveRef);

        }
        else if (strstr(input, "quit")) 
        {
            // terminating worker thread not possible in c++11 :-/
#if 0
            if (worker_thread && Game::searching)
            {
                delete worker_thread;
                worker_thread = NULL;
            }
#endif
            TranspositionTable::destroy();
            return;
        }
        else if (strstr(input, "dispboard")) 
        {
            HexaBitBoardPosition pos;
            Game::GetPos(&pos);
            Utils::dispBoard(&pos);
            uint64 hash = BitBoardUtils::ComputeZobristKey(&pos);
            uint32 high = (uint32) (hash >> 32);
            uint32 low  = (uint32)  (hash & 0xFFFFFFFF);
            printf("\nHash: 0x%X%X\n", high, low);
        }
        else if (strstr(input, "go")) 
        {
            // ready for some action
            input += 3;
            Search_Go(input);
        }
        else if (strstr(input, "bench")) 
        {
            printf("bench function TODO\n");
        }
        else if (strstr(input, "eval"))
        {
            HexaBitBoardPosition pos;
            Game::GetPos(&pos);
            int val = BitBoardUtils::Evaluate(&pos);
            printf("\nBoard eval: %d\n", val);
        }
        else if (strstr(input, "perft"))
        {
            input += 6;
            int depth = atoi(input);

            Timer timer;
            timer.start();
            uint64 val = Game::Perft(depth);
            uint64 time = timer.getElapsedMicroSeconds();
            uint64 nps = time ? val * 1000000 / time : 0;
            printf("perft %d: %llu, time: %llu us, nps: %llu\n", depth, val, time, nps);
        }
        else if (strstr(input, "eval"))
        {
            HexaBitBoardPosition curPos;
            Game::GetPos(&curPos);
            printf("Board Eval: %f\n", BitBoardUtils::Evaluate(&curPos));
        }
        else if (strstr(input, "stop")) 
        {
            if (worker_thread && Game::searching)
            {
                // crap... there is no way to forcefully terminate a C++11 thread :-/
                // delete worker_thread;
                Game::SetTimeControls(0, 0, 40, 0, 0, 1);
                worker_thread = NULL;
            }
            // stop the current line of search, and display the best move found
        }
        fflush(stdout);
    }
}


// static variables definations
std::thread* UciInterface::worker_thread;