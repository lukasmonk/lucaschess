#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rodent.h"
#include "timer.h"
#include "book.h"
#include "eval.h"
#include "param.h"

void ReadLine(char *str, int n) {
  char *ptr;

  if (fgets(str, n, stdin) == NULL)
    exit(0);
  if ((ptr = strchr(str, '\n')) != NULL)
    *ptr = '\0';
}

char *ParseToken(char *string, char *token) {

  while (*string == ' ')
    string++;
  while (*string != ' ' && *string != '\0')
    *token++ = *string++;
  *token = '\0';
  return string;
}

void UciLoop(void) {

  char command[4096], token[180], *ptr;
  POS p[1];

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  SetPosition(p, START_POS);
  AllocTrans(16);
  for (;;) {
    ReadLine(command, sizeof(command));
    ptr = ParseToken(command, token);

    // checks if Rodent should play with an opening book
    // UseBook remains for backward compatibly
    if ((strstr(command, "setoption name OwnBook value")) || (strstr(command, "setoption name UseBook value")))
      use_book = (strstr(command, "value true") != 0);
    if (strstr(command, "setoption name UCI_LimitStrength value"))
      Param.fl_weakening = (strstr(command, "value true") != 0);

    if (strcmp(token, "uci") == 0) {
      printf("id name %s\n", PROG_NAME);
      printf("id author Pawel Koziol (based on Sungorus 1.4 by Pablo Vazquez)\n");
      printf("option name Hash type spin default 16 min 1 max 4096\n");
      printf("option name Clear Hash type button\n");
      if (panel_style > 0) {
        printf("option name PawnValue type spin default %d min 0 max 1200\n", Param.pc_value[P]);
        printf("option name KnightValue type spin default %d min 0 max 1200\n", Param.pc_value[N]);
        printf("option name BishopValue type spin default %d min 0 max 1200\n", Param.pc_value[B]);
        printf("option name RookValue type spin default %d min 0 max 1200\n", Param.pc_value[R]);
        printf("option name QueenValue type spin default %d min 0 max 1200\n", Param.pc_value[Q]);
        
		printf("option name KeepPawn type spin default %d min -200 max 200\n", Param.keep_pc[P]);
        printf("option name KeepKnight type spin default %d min -200 max 200\n", Param.keep_pc[N]);
        printf("option name KeepBishop type spin default %d min -200 max 200\n", Param.keep_pc[B]);
        printf("option name KeepRook type spin default %d min -200 max 200\n", Param.keep_pc[R]);
        printf("option name KeepQueen type spin default %d min -200 max 200\n", Param.keep_pc[Q]);

        printf("option name BishopPair type spin default %d min -100 max 100\n", Param.bish_pair);
        if (panel_style == 2)
			printf("option name KnightPair type spin default %d min -100 max 100\n", Param.knight_pair);
           printf("option name ExchangeImbalance type spin default %d min -100 max 100\n", Param.exchange_imbalance);
        printf("option name KnightLikesClosed type spin default %d min 0 max 10\n", Param.np_bonus);
        if (panel_style == 2)
           printf("option name RookLikesOpen type spin default %d min 0 max 10\n", Param.rp_malus);

        printf("option name Material type spin default %d min 0 max 500\n", Param.mat_perc);
        printf("option name OwnAttack type spin default %d min 0 max 500\n", dyn_weights[DF_OWN_ATT]);
        printf("option name OppAttack type spin default %d min 0 max 500\n", dyn_weights[DF_OPP_ATT]);
        printf("option name OwnMobility type spin default %d min 0 max 500\n", dyn_weights[DF_OWN_MOB]);
        printf("option name OppMobility type spin default %d min 0 max 500\n", dyn_weights[DF_OPP_MOB]);

        printf("option name KingTropism type spin default %d min -50 max 500\n", weights[F_TROPISM]);
        printf("option name PiecePlacement type spin default %d min 0 max 500\n", Param.pst_perc);
        printf("option name PiecePressure type spin default %d min 0 max 500\n", weights[F_PRESSURE]);
        printf("option name PassedPawns type spin default %d min 0 max 500\n", weights[F_PASSERS]);
        printf("option name PawnStructure type spin default %d min 0 max 500\n", weights[F_PAWNS]);

		printf("option name Outposts type spin default %d min 0 max 500\n", weights[F_OUTPOST]);
        printf("option name Lines type spin default %d min 0 max 500\n", weights[F_LINES]);
        if (panel_style == 2) {
          printf("option name PawnShield type spin default %d min 0 max 500\n", Param.shield_perc);
          printf("option name PawnStorm type spin default %d min 0 max 500\n", Param.storm_perc);
		  printf("option name Forwardness type spin default %d min 0 max 500\n", Param.forwardness);
        }
        printf("option name PstStyle type spin default %d min 0 max 2\n", Param.pst_style);
		printf("option name MobilityStyle type spin default %d min 0 max 1\n", Param.mob_style);

        if (panel_style == 2) {
          printf("option name DoubledPawnMg type spin default %d min -100 max 0\n", Param.doubled_malus_mg);
          printf("option name DoubledPawnEg type spin default %d min -100 max 0\n", Param.doubled_malus_eg);
          printf("option name IsolatedPawnMg type spin default %d min -100 max 0\n", Param.isolated_malus_mg);
          printf("option name IsolatedPawnEg type spin default %d min -100 max 0\n", Param.isolated_malus_eg);
          printf("option name IsolatedOnOpenMg type spin default %d min -100 max 0\n", Param.isolated_open_malus);
          printf("option name BackwardPawnMg type spin default %d min -100 max 0\n", Param.backward_malus_base);
          printf("option name BackwardPawnEg type spin default %d min -100 max 0\n", Param.backward_malus_eg);
          printf("option name BackwardOnOpenMg type spin default %d min -100 max 0\n", Param.backward_open_malus);
        }

        // Strength settings - we use either Elo slider with an approximate formula
        // or separate options for nodes per second reduction and eval blur

        if (fl_elo_slider == 0) {
          printf("option name NpsLimit type spin default %d min 0 max 5000000\n", Timer.nps_limit);
          printf("option name EvalBlur type spin default %d min 0 max 5000000\n", Param.eval_blur);
        } else {
          printf("option name UCI_LimitStrength type check default false\n");
          printf("option name UCI_Elo type spin default %d min 800 max 2800\n", Param.elo);
        }

        printf("option name Contempt type spin default %d min -250 max 250\n", Param.draw_score);
        printf("option name SlowMover type spin default %d min 10 max 500\n", time_percentage);
        printf("option name Selectivity type spin default %d min 0 max 200\n", hist_perc);
        printf("option name OwnBook type check default true\n");
        printf("option name GuideBookFile type string default guide.bin\n");
        printf("option name MainBookFile type string default rodent.bin\n");
        printf("option name BookFilter type spin default %d min 0 max 5000000\n", Param.book_filter);
     }

     if (panel_style == 0) {
        printf("option name PersonalityFile type string default rodent.txt\n");
        printf("option name OwnBook type check default true\n");
        if (fl_separate_books) {
          printf("option name GuideBookFile type string default guide.bin\n");
          printf("option name MainBookFile type string default rodent.bin\n");
        }
     }

      printf("uciok\n");
    } else if (strcmp(token, "isready") == 0) {
      printf("readyok\n");
    } else if (strcmp(token, "setoption") == 0) {
      ParseSetoption(ptr);
    } else if (strcmp(token, "position") == 0) {
      ParsePosition(p, ptr);
    } else if (strcmp(token, "perft") == 0) {
      ptr = ParseToken(ptr, token);
    int depth = atoi(token);
    if (depth == 0) depth = 5;
    Timer.SetStartTime();
    nodes = Perft(p, 0, depth);
#if defined _WIN32 || defined _WIN64 
    printf (" perft %d : %I64d nodes in %d miliseconds\n", depth, nodes, Timer.GetElapsedTime() );
#else
    printf(" perft %d : %lld nodes in %d miliseconds\n", depth, nodes, Timer.GetElapsedTime());
#endif
    } else if (strcmp(token, "print") == 0) {
      PrintBoard(p);
    } else if (strcmp(token, "eval") == 0) {
      SetAsymmetricEval(p->side);
      Eval.Print(p);
    } else if (strcmp(token, "step") == 0) {
      ParseMoves(p, ptr);
    } else if (strcmp(token, "go") == 0) {
      ParseGo(p, ptr);
    } else if (strcmp(token, "bench") == 0) {
      ptr = ParseToken(ptr, token);
      Bench(atoi(token));
    } else if (strcmp(token, "quit") == 0) {
      return;
    }
  }
}

void ParseSetoption(char *ptr) {

  char token[180], name[180], value[180] = "";

  ptr = ParseToken(ptr, token);
  name[0] = '\0';
  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0' || strcmp(token, "value") == 0)
      break;
    strcat(name, token);
    strcat(name, " ");
  }
  name[strlen(name) - 1] = '\0';
  if (strcmp(token, "value") == 0) {
    value[0] = '\0';

    for (;;) {
      ptr = ParseToken(ptr, token);
      if (*token == '\0')
        break;
      strcat(value, token);
      strcat(value, " ");
    }
    value[strlen(value) - 1] = '\0';
  }

  if (strcmp(name, "Hash") == 0) {
    AllocTrans(atoi(value));
  } else if (strcmp(name, "Clear Hash") == 0 || strcmp(name, "clear hash") == 0) {
    ResetEngine();
  } else if (strcmp(name, "Material") == 0 || strcmp(name, "material") == 0) {
    Param.mat_perc = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "PiecePlacement") == 0 || strcmp(name, "pieceplacement") == 0) {
    Param.pst_perc = (pst_default_perc[Param.pst_style] * atoi(value)) / 100; // scaling takes into account internal weight
    Param.DynamicInit();
  } else if (strcmp(name, "PawnValue") == 0   || strcmp(name, "pawnvalue") == 0) {
    Param.pc_value[P] = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "KnightValue") == 0       || strcmp(name, "knightvalue") == 0) {
    Param.pc_value[N] = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "BishopValue") == 0       || strcmp(name, "bishopvalue") == 0) {
    Param.pc_value[B] = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "RookValue") == 0         || strcmp(name, "rookvalue") == 0) {
    Param.pc_value[R] = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "QueenValue") == 0        || strcmp(name, "queenvalue") == 0) {
    Param.pc_value[Q] = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "KeepQueen") == 0         || strcmp(name, "keepqueen") == 0) {
    Param.keep_pc[Q] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KeepRook") == 0          || strcmp(name, "keeprook") == 0) {
    Param.keep_pc[R] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KeepBishop") == 0        || strcmp(name, "keepbishop") == 0) {
    Param.keep_pc[B] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KeepKnight") == 0        || strcmp(name, "keepknight") == 0) {
    Param.keep_pc[N] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KeepPawn") == 0          || strcmp(name, "keeppawn") == 0) {
    Param.keep_pc[P] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KnightLikesClosed") == 0 || strcmp(name, "knightlikesclosed") == 0) {
    Param.np_bonus = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "RookLikesOpen") == 0     || strcmp(name, "rooklikesopen") == 0) {
    Param.rp_malus = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "OwnAttack") == 0         || strcmp(name, "ownattack") == 0) {
    dyn_weights[DF_OWN_ATT] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "OppAttack") == 0         || strcmp(name, "oppattack") == 0) {
    dyn_weights[DF_OPP_ATT] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "OwnMobility") == 0       || strcmp(name, "ownmobility") == 0) {
    dyn_weights[DF_OWN_MOB] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "OppMobility") == 0       || strcmp(name, "oppmobility") == 0) {
    dyn_weights[DF_OPP_MOB] = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "KingTropism") == 0       || strcmp(name, "kingtropism") == 0) {
    SetWeight(F_TROPISM, atoi(value));
  } else if (strcmp(name, "PiecePressure") == 0     || strcmp(name, "piecepressure") == 0) {
    SetWeight(F_PRESSURE, atoi(value));
  } else if (strcmp(name, "PassedPawns") == 0       || strcmp(name, "passedpawns") == 0) {
    SetWeight(F_PASSERS, atoi(value));
  } else if (strcmp(name, "PawnStructure") == 0     || strcmp(name, "pawnstructure") == 0) {
    SetWeight(F_PAWNS, atoi(value));
  } else if (strcmp(name, "Lines") == 0             || strcmp(name, "lines") == 0) {
   SetWeight(F_LINES, atoi(value));
  } else if (strcmp(name, "Outposts") == 0          || strcmp(name, "outposts") == 0) {
    SetWeight(F_OUTPOST, atoi(value));
  } else if (strcmp(name, "PstStyle") == 0          || strcmp(name, "pststyle") == 0) {
    Param.pst_style = atoi(value);
    Param.DynamicInit();
 } else if (strcmp(name, "MobilityStyle") == 0      || strcmp(name, "mobilitystyle") == 0) {
    Param.mob_style = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "ExchangeImbalance") == 0 || strcmp(name, "exchangeimbalance") == 0) {
    Param.exchange_imbalance = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "BishopPair") == 0        || strcmp(name, "bishoppair") == 0) {
    Param.bish_pair = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "DoubledPawnMg") == 0     || strcmp(name, "doubledpawnmg") == 0) {
    Param.doubled_malus_mg = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "DoubledPawnEg") == 0     || strcmp(name, "doubledpawneg") == 0) {
    Param.doubled_malus_eg = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "IsolatedPawnMg") == 0    || strcmp(name, "isolatedpawnmg") == 0) {
    Param.isolated_malus_mg = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "IsolatedPawnEg") == 0    || strcmp(name, "isolatedpawneg") == 0) {
    Param.isolated_malus_eg = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "IsolatedOnOpenMg") == 0  || strcmp(name, "isolatedonopenmg") == 0) {
    Param.isolated_open_malus = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "BackwardPawnMg") == 0    || strcmp(name, "backwardpawnmg") == 0) {
    Param.backward_malus_base = atoi(value);
    Param.DynamicInit();
  } else if (strcmp(name, "BackwardPawnEg") == 0    || strcmp(name, "backwardpawneg") == 0) {
    Param.backward_malus_eg = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "BackwardOnOpenMg") == 0  || strcmp(name, "backwardonopenmg") == 0) {
    Param.backward_open_malus = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "PawnShield") == 0        || strcmp(name, "pawnshield") == 0) {
    Param.shield_perc = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "PawnStorm") == 0         || strcmp(name, "pawnstorm") == 0) {
    Param.storm_perc = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "Forwardness") == 0       || strcmp(name, "forwardness") == 0) {
    Param.forwardness = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "NpsLimit") == 0          || strcmp(name, "npslimit") == 0) {
    Timer.nps_limit = atoi(value);
    ResetEngine();
    if (Timer.nps_limit != 0) Param.fl_weakening = 1;
  } else if (strcmp(name, "EvalBlur") == 0          || strcmp(name, "evalblur") == 0) {
    Param.eval_blur = atoi(value);
    ResetEngine();
    if (Param.eval_blur != 0) Param.fl_weakening = 1;
  } else if (strcmp(name, "Contempt") == 0          || strcmp(name, "contempt") == 0) {
    Param.draw_score = atoi(value);
    ResetEngine();
  } else if (strcmp(name, "SlowMover") == 0         || strcmp(name, "slowmover") == 0) {
    time_percentage = atoi(value);
  } else if (strcmp(name, "UCI_Elo") == 0           || strcmp(name, "uci_elo") == 0) {
    Param.elo = atoi(value);
    Timer.SetSpeed(Param.elo);
  } else if (strcmp(name, "Selectivity") == 0       || strcmp(name, "selectivity") == 0) {
    hist_perc = atoi(value);
    hist_limit = -HIST_LIMIT + ((HIST_LIMIT * hist_perc) / 100);
  } else if (strcmp(name, "GuideBookFile") == 0     || strcmp(name, "guidebookfile") == 0) {
    if (!fl_separate_books || !fl_reading_personality) {
      GuideBook.ClosePolyglot();
      GuideBook.bookName = value;
      GuideBook.OpenPolyglot();
    }
  } else if (strcmp(name, "MainBookFile") == 0      || strcmp(name, "mainbookfile") == 0) {
    if (!fl_separate_books || !fl_reading_personality) {
      MainBook.ClosePolyglot();
      MainBook.bookName = value;
      MainBook.OpenPolyglot();
    }
  } else if (strcmp(name, "PersonalityFile") == 0   || strcmp(name, "personalityfile") == 0) {
    printf("info string reading ");
    printf(value);
    printf("\n");
    ReadPersonality(value);
  } else if (strcmp(name, "BookFilter") == 0        || strcmp(name, "bookfilter") == 0) {
    Param.book_filter = atoi(value);
  }
}

void SetWeight(int weight_name, int value) {

  weights[weight_name] = value;
  ResetEngine();
}

void ParseMoves(POS *p, char *ptr) {
  
  char token[180];
  UNDO u[1];

  for (;;) {

    // Get next move to parse

    ptr = ParseToken(ptr, token);

  // No more moves!

    if (*token == '\0') break;

    p->DoMove(StrToMove(p, token), u);

  // We won't be taking back moves beyond this point:

    if (p->rev_moves == 0) p->head = 0;
  }
}

void ParsePosition(POS *p, char *ptr) {

  char token[180], fen[180];

  ptr = ParseToken(ptr, token);
  if (strcmp(token, "fen") == 0) {
    fen[0] = '\0';
    for (;;) {
      ptr = ParseToken(ptr, token);

      if (*token == '\0' || strcmp(token, "moves") == 0)
        break;

      strcat(fen, token);
      strcat(fen, " ");
    }
    SetPosition(p, fen);
  } else {
    ptr = ParseToken(ptr, token);
    SetPosition(p, START_POS);
  }

  if (strcmp(token, "moves") == 0)
    ParseMoves(p, ptr);
}

void ParseGo(POS *p, char *ptr) {

  char token[180], bestmove_str[6], ponder_str[6];
  int pv[MAX_PLY];

  Timer.Clear();
  pondering = 0;

  for (;;) {
    ptr = ParseToken(ptr, token);
    if (*token == '\0')
      break;
    if (strcmp(token, "ponder") == 0) {
      pondering = 1;
    } else if (strcmp(token, "wtime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_TIME, atoi(token));
    } else if (strcmp(token, "btime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_TIME, atoi(token));
    } else if (strcmp(token, "winc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(W_INC, atoi(token));
    } else if (strcmp(token, "binc") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(B_INC, atoi(token));
    } else if (strcmp(token, "movestogo") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(MOVES_TO_GO, atoi(token));
    } else if (strcmp(token, "nodes") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(FLAG_INFINITE, 1);
      Timer.SetData(MAX_NODES, atoi(token));
    } else if (strcmp(token, "movetime") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(MOVE_TIME, atoi(token) );
    } else if (strcmp(token, "depth") == 0) {
      ptr = ParseToken(ptr, token);
      Timer.SetData(FLAG_INFINITE, 1);
      Timer.SetData(MAX_DEPTH, atoi(token));
    } else if (strcmp(token, "infinite") == 0) {
      Timer.SetData(FLAG_INFINITE, 1);
    }
  }

  Timer.SetSideData(p->side);
  Timer.SetMoveTiming();
  Think(p, pv);
  MoveToStr(pv[0], bestmove_str);
  if (pv[1]) {
    MoveToStr(pv[1], ponder_str);
    printf("bestmove %s ponder %s\n", bestmove_str, ponder_str);
  } else
    printf("bestmove %s\n", bestmove_str);
}

void ResetEngine(void) {

  ClearHist();
  ClearTrans();
  ClearEvalHash();
  ClearPawnHash();
}

void ReadPersonality(char *fileName)
{
  FILE *personalityFile;
  char line[256];
  int lineNo = 0;
  char token[180], *ptr;

  // exit if this personality file doesn't exist
  if ((personalityFile = fopen(fileName, "r")) == NULL)
    return;

  fl_reading_personality = 1;

  // read options line by line

  while (fgets(line, 256, personalityFile)) {
    ptr = ParseToken(line, token);

    if (strstr(line, "HIDE_OPTIONS")) panel_style = 0;
    if (strstr(line, "SHOW_OPTIONS")) panel_style = 1;
    if (strstr(line, "FULL_OPTIONS")) panel_style = 2;

    if (strstr(line, "PERSONALITY_BOOKS")) fl_separate_books = 0;
    if (strstr(line, "GENERAL_BOOKS")) fl_separate_books = 1;

    if (strstr(line, "ELO_SLIDER")) fl_elo_slider = 1;
    if (strstr(line, "NPS_BLUR")) fl_elo_slider = 0;

    if (strcmp(token, "setoption") == 0)
      ParseSetoption(ptr);
  }

  fclose(personalityFile);
  fl_reading_personality = 0;
}

int Perft(POS *p, int ply, int depth) {

  int move = 0;
  int fl_mv_type;
  MOVES m[1];
  UNDO u[1];
  int mv_cnt = 0;

  InitMoves(p, m, 0, 0, ply);

  while (move = NextMove(m, &fl_mv_type)) {

  p->DoMove(move, u);

  if (Illegal(p)) { p->UndoMove(move, u); continue; }

  if (depth == 1) mv_cnt++;
    else          mv_cnt += Perft(p, ply + 1, depth - 1);
    p->UndoMove(move, u);
  }

  return mv_cnt;
}

void PrintBoard(POS *p) {

  char *piece_name[] = { "P ", "p ", "N ", "n ", "B ", "b ", "R ", "r ", "Q ", "q ", "K ", "k ", ". " };

  printf("--------------------------------------------\n");
  for (int sq = 0; sq < 64; sq++) {
    printf(piece_name[p->pc[sq ^ (BC * 56)]]);
    if ((sq + 1) % 8 == 0) printf(" %d\n", 9 - ((sq + 1) / 8));
  }

  printf("\na b c d e f g h\n\n--------------------------------------------\n");
}

void Bench(int depth) {

  POS p[1];
  int pv[MAX_PLY];
  char *test[] = {
    "r1bqkbnr/pp1ppppp/2n5/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
    "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
    "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
    "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
    "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
    "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
    "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
    "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
    "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
    "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
    "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
    "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
    NULL
  }; // test positions taken from DiscoCheck by Lucas Braesch

  if (depth == 0) depth = 8; // so that you can call bench without parameters

  printf("Bench test started (depth %d): \n", depth);

  ResetEngine();
  nodes = 0;
  verbose = 0;
  Timer.SetData(MAX_DEPTH, depth);
  Timer.SetData(FLAG_INFINITE, 1);
  Timer.SetStartTime();

  for (int i = 0; test[i]; ++i) {
    printf(test[i]);
    SetPosition(p, test[i]);
    printf("\n");
    Iterate(p, pv);
  }

  int end_time = Timer.GetElapsedTime();
  int nps = (nodes * 1000) / (end_time + 1);

  printf("%llu nodes searched in %d, speed %u nps (Score: %.3f)\n", nodes, end_time, nps, (float)nps / 430914.0);
}
