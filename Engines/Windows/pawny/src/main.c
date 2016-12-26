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
#include <math.h>

void init()
{
  suppress_buffering();
  //initialize:
  board_init();
  tt_init(DEFAULT_TT_SIZE);
  pnt_init();
  opt->analyze = false;
  opt->time_high = 50000;//50 sec. default max time/move
  opt->time_low  = 30000;
  opt->aspiration = DEFAULT_ASPIRATION_WINDOW;
  opt->rtp = false;
  init_distance();
  init_direction();
  init_attack_vector();
  memset(si,0,sizeof(searchinfo_t));
  book_open();
}

int main(int argc, char *argv[])
{///program entry point
  int i;
  char sInput[512];
  char *p;
  move_t move;

  init();
  
  //parsing the cmd line:
  for(i = 1; i < argc; i++) parse_input(argv[i]);
  while(true)
  { p = fgets(sInput,sizeof(sInput),stdin);
    //null terminating:
    while(*p != '\n') p++;
    *p = '\0';

    //parse:
    if(!parse_input(sInput))
    { if(parse_move(sInput, &move))
      { if(!make_if_legal(move))
        { printf("Illegal move\n\n");
          continue;
        }
        opt->comp_side = board->side;
        think();	
      }
      else
      { printf("Unknown option: %s",sInput);
        printf("\n\n");
        continue;
      }
    }
  }
}
  

bool parse_input(char *sInput)
{
  int x;
  char strbuff[256];
  bool result = false;
  
  if(*sInput == '\0') return true;
  else if(!strcmp(sInput, "uci")) uci();
 
  else if(!strncmp(sInput, "hash", 4))
  { tt_init(atoi(sInput + 5));
    return true;
  }
  
  else if(!strcmp(sInput,"help"))
  { printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
    "\nPawny chess engine\nby Mincho Georgiev (c)2009-2011\n\nConsole mode commands:\n",
    "----------------------------\n",
    "- uci - entering uci mode.\n",
    "- hash <size> - determines the hash size in MB.(min. 8MB, max. 1024MB)\n",
    "- quit - exit(0);\n",
    "- go - force engine to start searching.\n",
    "- undo - undo the last move.\n",
    "- new - new game.\n",
    "- perft <depth> - performance test.\n",
    "- divide <depth> - divided perft.\n",
    "- setboard <fen> - sets the position from FEN string\n",
    "- getfen - display the FEN string, coresponding to the current pos.\n",
    "- sd <depth> - maximum search depth (time given is suppressed)\n",
    "- st <time> - maximum time per move (seconds)\n",
    "- version - displays current version.\n",
    "note: the moves are parsed in algebratic notation ('e2e4' 'e7e8q || e7e8Q', 'e1g1' instead of 'O-O')\n",
    "note: the default initial time per move is 50 sec.\n",
    "note: perft and divide are using another hash table with constant size of 25MB.\n\n");
    result = true;
  }
  
  else if(!strcmp(sInput,"version"))
  { printf("%s %s\n\n",ENGINE_NAME,VERSION);
    result = true;
  }
  
  else if(!strcmp(sInput,"quit"))
  { book_close();
    tt_free();
    exit(0);
  }

  else if(!strcmp(sInput,"go"))
  { opt->comp_side = board->side;
    think();
    result = true;
  }
  
  else if(!strcmp(sInput,"undo"))
  { move_undo();
    result = true;
  }

  else if(!strcmp(sInput,"new"))
  { board_from_fen(INITIAL_POSITION);
    result = true;
  }

  else if(!strncmp(sInput, "epdtest",7))
  { sInput += sizeof("epdtest");
    while(isspace(*sInput)) sInput++;
    if(!test_epd(sInput))
      printf ("EPD error!\n");
    result = true;
  }
  
  else if(strstr(sInput,"perft"))
  { sInput += sizeof("perft");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      test_perft(x);
    }
    result = true;
  }
  
  else if(strstr(sInput,"divide"))
  { sInput += sizeof("divide");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      test_divide(x);
    }
    result = true;
  }

  else if(strstr(sInput,"pawndiv"))
  { sInput += sizeof("pawndiv");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      test_pawn_divide(x);
    }
    result = true;
  }
    
  else if(!strcmp(sInput,"getfen"))
  { board_to_fen(&strbuff[0]);
    printf("\n");
    printf("%s",strbuff);
    printf("\n\n");
    result = true;
  }
  
  else if(strstr(sInput,"setboard"))
  { sInput += sizeof("setboard");
    while(isspace(*sInput)) sInput++;
    if(!board_from_fen(sInput))
      printf ("Illegal position (FEN) !\n");
    result = true;
  }
  
  else if(strstr(sInput,"sd"))
  { sInput += sizeof("sd");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      opt->max_depth = x;
      opt->analyze = true;
    }
    result = true;
  }
  
  else if(strstr(sInput,"st"))
  { sInput += sizeof("st");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      opt->time_high = x*1000;
      opt->analyze = false;
      opt->time_fixed = true;
    }
    result = true;
  }
  
  else if(!strcmp(sInput,"eval"))
  { int r;float score;
    r = eval();
    if(opt->comp_side == BLACK && r != 0)
      score = -(r / ((float)100));
    else
      score = (r / ((float)100));
    printf(" %.2f \n",score);
    result = true;
  }
  
  return result;
}

bool parse_move(char *sInput,move_t *move)
{///parsing the input as move string
///for now just a simple "e2e4" notation.
  int file,rank;
  char c = (char)tolower(*sInput);
    
  memset(move,0,sizeof(move_t));
  
  //determine what the first char is
  switch(c)
  {
    //files
    case 'a': file = FILE_A; break;
    case 'b': file = FILE_B; break;
    case 'c': file = FILE_C; break;
    case 'd': file = FILE_D; break;
    case 'e': file = FILE_E; break;
    case 'f': file = FILE_F; break;
    case 'g': file = FILE_G; break;
    case 'h': file = FILE_H; break;
    default: return false; break;
  }

  //the second char:
  sInput++;
  switch(*sInput)
  {
    case '1': rank = RANK_1; break;
    case '2': rank = RANK_2; break;
    case '3': rank = RANK_3; break;
    case '4': rank = RANK_4; break;
    case '5': rank = RANK_5; break;
    case '6': rank = RANK_6; break;
    case '7': rank = RANK_7; break;
    case '8': rank = RANK_8; break;
    default: return false; break;
  }

  //the third char
  sInput++;
  move->from = (sq_t)calc_sq(file, rank);
    
  c = (char)tolower(*sInput);
  switch(c)
  {
    //files
    case 'a': file = FILE_A; break;
    case 'b': file = FILE_B; break;
    case 'c': file = FILE_C; break;
    case 'd': file = FILE_D; break;
    case 'e': file = FILE_E; break;
    case 'f': file = FILE_F; break;
    case 'g': file = FILE_G; break;
    case 'h': file = FILE_H; break;
    default: return false; break;
  }

  //the fourth char	
  sInput++;
  switch(*sInput)
  {
    case '1': rank = RANK_1; break;
    case '2': rank = RANK_2; break;
    case '3': rank = RANK_3; break;
    case '4': rank = RANK_4; break;
    case '5': rank = RANK_5; break;
    case '6': rank = RANK_6; break;
    case '7': rank = RANK_7; break;
    case '8': rank = RANK_8; break;
    default: return false; break;
  }
  
  sInput++;
  move->to = (sq_t)calc_sq(file, rank);
  //is there a 5th charachter (promotion) ?
  if(*sInput) 
  {
    c = (char)tolower(*sInput);
    switch(c)
    {
      case 'q': move->promoted = (uint8)Coloured(QUEEN); break;
      case 'n': move->promoted = (uint8)Coloured(KNIGHT); break;
      case 'r': move->promoted = (uint8)Coloured(ROOK); break;
      case 'b': move->promoted = (uint8)Coloured(BISHOP); break;
      case '\n': case ' ': break;
      default : return false; break;
    }
  }
  
  return true;
}

void print_move(uint32 move)
{///simple procedure to print a given move:
  int file,rank;
  char mstr[8];
  char *c = &mstr[0];
  move_data_t m;
  
  m.p = move;
  file = calc_file(m.from);
  rank = calc_rank(m.from);
  
  switch(file)
  {
    case FILE_A: *c = 'a'; break;
    case FILE_B: *c = 'b'; break;
    case FILE_C: *c = 'c'; break;
    case FILE_D: *c = 'd'; break;
    case FILE_E: *c = 'e'; break;
    case FILE_F: *c = 'f'; break;
    case FILE_G: *c = 'g'; break;
    case FILE_H: *c = 'h'; break;
    default: break;
  }
  
  c++;
  switch(rank)
  {
    case RANK_1: *c = '1'; break;
    case RANK_2: *c = '2'; break;
    case RANK_3: *c = '3'; break;
    case RANK_4: *c = '4'; break;
    case RANK_5: *c = '5'; break;
    case RANK_6: *c = '6'; break;
    case RANK_7: *c = '7'; break;
    case RANK_8: *c = '8'; break;
  }
  
  c++;
  file = calc_file(m.to);
  rank = calc_rank(m.to);
  switch(file)
  {
    case FILE_A: *c = 'a'; break;
    case FILE_B: *c = 'b'; break;
    case FILE_C: *c = 'c'; break;
    case FILE_D: *c = 'd'; break;
    case FILE_E: *c = 'e'; break;
    case FILE_F: *c = 'f'; break;
    case FILE_G: *c = 'g'; break;
    case FILE_H: *c = 'h'; break;
    default: break;
  }
  
  c++;
  switch(rank)
  {
    case RANK_1: *c = '1'; break;
    case RANK_2: *c = '2'; break;
    case RANK_3: *c = '3'; break;
    case RANK_4: *c = '4'; break;
    case RANK_5: *c = '5'; break;
    case RANK_6: *c = '6'; break;
    case RANK_7: *c = '7'; break;
    case RANK_8: *c = '8'; break;
  }

  c++;
  switch(GetType(m.promoted))
  {
    case QUEEN:  *c = 'Q'; c++; break;
    case ROOK:   *c = 'R'; c++; break;
    case BISHOP: *c = 'B'; c++; break;	
    case KNIGHT: *c = 'N'; c++; break;
    default: break;
  }
  
  *c = '\0';
  printf("%s ",mstr);
}

