/*--------------------------------------------------------------------------
    Pawny 0.3.1, chess engine (source code).
    Copyright (C) 2009 - 2011 by Mincho Georgiev.
    
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
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#include "data.h"

void uci_start()
{
  printf("id name %s %s\n",ENGINE_NAME, VERSION);
  printf("id author Mincho Georgiev\n");
  printf("option name Hash type spin default 32 min 8 max 1024\n");
  printf("option name Reset Timer On Ponderhit type check default false\n");
  printf("option name OwnBook type check default true\n");
  printf("option name Ponder type check default false\n");
  #ifdef GTB
  printf("option name Gaviota Tablebases type check default false\n");
  printf("option name GTB Path type string\n");
  printf("option name GTB Cache Size type spin default 32 min 8 max 1024\n");
  printf("option name GTB Compression (0-9) type spin default 4 min 0 max 9\n");
  gtb_uci = false;
  gtb_ok = false;
  #endif
  printf("uciok\n");
}

void uci_exit()
{ book_close();
  tt_free(); 
  #ifdef GTB
  gtb_clear();
  #endif
  exit(0);
}

void uci_cmd_handle(char *command)
{
  char *p = 0;

  if(!strncmp(command, "isready",7)) printf("readyok\n");
  else if(!strncmp(command, "quit", 4)) uci_exit();
  else if(!strncmp(command, "position", 8)) uci_set_position(command + 9);
  else if(!strncmp(command, "ucinewgame", 10)) uci_new_game();
  else if(!strncmp(command, "setoption name Hash value", 25)) tt_init(atoi(command + 26));
  
  #ifdef GTB
  else if(!strncmp(command, "setoption name Gaviota Tablebases", 33)) 
  {  if(!strncmp(command + 34, "value true", 10))
    { gtb_uci = true;
      gtb_cachesize  = GTB_CACHE_DEFAULT*1024*1024;
      gtb_dec_scheme = GTB_SCHEME_DEFAULT;
    }
  }
  else if(!strncmp(command, "setoption name GTB Path", 23))
  { if(*(command + 30) != 0)
    { strcpy(gtb_path, (command + 30));
      gtb_path[strlen(command+30)-1] = 0;
    }
    if(gtb_uci == true) gtb_init();
  }
  else if(!strncmp(command, "setoption name GTB Cache Size", 29))
  { gtb_cachesize = (atoi(command + 36)) * 1024 * 1024;
    if(gtb_uci == true) gtb_init();
  }
  else if(!strncmp(command, "setoption name GTB Compression (0-9)", 36))
  { gtb_dec_scheme = atoi(command + 43);
    if(gtb_uci == true) gtb_init();
  }
  #endif
  
  else if(!strncmp(command, "go", 2)) uci_go(command + 3);
  else if(!strncmp(command, "stop", 4)) si->stop_search = true;
  p = strstr(command, "Reset Timer On Ponderhit");
  if(p)
  { if(!strncmp(p + 25, "value true", 10))  opt->rtp = true;
    if(!strncmp(p + 25, "value false", 11)) opt->rtp = false;
  }
  p = strstr(command, "OwnBook");
  if(p)
  { if(!strncmp(p + 8, "value true", 10))  book_open();
    if(!strncmp(p + 8, "value false", 11)) book_close();
  }
}

void uci()
{
  char line[4096];
  uci_start();
  
  opt->mode = UCI_MODE;
  while(true)
  { if(!fgets(&line[0], sizeof(line), stdin)) 
      uci_exit();
    uci_cmd_handle(line);
  }
}

void uci_new_game(){board_init();}

void uci_go(char *commands)
{
  char *startchar = 0;
  int time[2];
  int t, inc;
  int time_inc[2];
  int movestogo = 0;
  int movetime = 0;
  char *ponder   = strstr(commands, "ponder");
  char *infinite = strstr(commands, "infinite");

  opt->max_depth = MAX_DEPTH;
  startchar = strstr(commands, "depth");
  if(startchar)
  { sscanf(startchar + 6, "%d", &opt->max_depth);
    opt->analyze = true;
    goto l1;
  }
  
  opt->time_fixed = false;
  time[0] = 0; time[1] = 0; time_inc[0] = 0; time_inc[1] = 0;
  
  startchar = strstr(commands, "wtime");
  if(startchar) sscanf(startchar + 6, "%d", &time[WHITE]);
  startchar = strstr(commands, "btime");
  if(startchar) sscanf(startchar + 6, "%d", &time[BLACK]);
  startchar = strstr(commands, "winc");
  if(startchar) sscanf(startchar + 5, "%d", &time_inc[WHITE]);
  startchar = strstr(commands, "binc");
  if(startchar) sscanf(startchar + 5, "%d",  &time_inc[BLACK]);
  startchar = strstr(commands, "movestogo");
  if(startchar) sscanf(startchar + 10, "%d", &movestogo);
  startchar = strstr(commands, "movetime");
  if(startchar) sscanf(startchar + 9, "%d", &movetime);
  
  t = time[board->side]; inc = time_inc[board->side];
  opt->analyze = (uint8)(ponder || infinite);
  
  if(movetime)
  { opt->time_fixed = true;
    opt->time_high  = movetime - min(movetime/40, 100);
  }
  else if(movestogo)
  { if(movestogo == 1)
    { //shorter time for the last move
      //to avoid GUI delays or exceeded time limit:
      opt->time_low  = t / 2;
      opt->time_high = min(t / 2, t - 500);
    }
    else
    { if(inc)
      { opt->time_low  = t / min(movestogo, 30) + inc;
        opt->time_high = min((t * 4) / movestogo, t / 3) + inc;
      }
      else
      { opt->time_low  = t / min(movestogo, 20);
        opt->time_high = min((t * 4) / movestogo, t / 3);
      }
    }
  }
  else
  { if(inc)
    { opt->time_low  = t / 30 + inc;
      opt->time_high = max(t / 4, inc - 100);
    }
    else
    { opt->time_low  = t / 40;
      opt->time_high = t / 8;
    }
  }
  l1:
  think();
}

void uci_set_position(char *pstr)
{
  char *startpos;
  char *fen;
  char *moves;
  char  movestr[8];
  move_t move;
  int i;
  
  startpos = strstr(pstr, "startpos");
  fen = strstr(pstr, "fen");
  moves = strstr(pstr,"moves");

  if(startpos)
  { board_from_fen(INITIAL_POSITION);		
    if(!moves) return;
    ///else - moves from startup position maded:
    moves += 6;
    while(*moves)
    { //getting the next move string
      memset(&movestr, 0, sizeof(movestr));
      for(i = 0;!isspace(*moves);i++)
      { movestr[i] = *moves;
        moves++;
      }
      //string to move_t conversion:
      parse_move(&movestr[0], &move);
      //generate, find it and make it:
      if(!make_if_legal(move)) return;
      //get rid of the next space:
      if(isspace(*moves)) moves++;
    }
  }
  if(fen)
  { //setting the position first
    fen += 4;
    board_from_fen(fen);
    if(!moves) return;
    //else - getting to the present pos.
    moves += 6;
    while(*moves)
    { //getting the next move string
      memset(&movestr, 0, sizeof(movestr));
      for(i = 0;!isspace(*moves);i++)
      { movestr[i] = *moves;
        moves++;
      }
      //string to move_t conversion:
      parse_move(&movestr[0], &move);
      //generate, find it and make it:
      if(!make_if_legal(move)) return;
      //get rid of the next space:
      if(isspace(*moves)) moves++;
    }
  }
}
