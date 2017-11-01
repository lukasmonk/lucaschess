/*--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
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

void print_move(uint32 pm)
{
  int file,rank;
  char c = 'x';
  move_t m;

  m.p = pm;
  file = File(m.from);
  rank = Rank(m.from);

  switch(file)
  { case FILE_A: c = 'a'; break;
    case FILE_B: c = 'b'; break;
    case FILE_C: c = 'c'; break;
    case FILE_D: c = 'd'; break;
    case FILE_E: c = 'e'; break;
    case FILE_F: c = 'f'; break;
    case FILE_G: c = 'g'; break;
    case FILE_H: c = 'h'; break;
    default: break;
  }
  printf("%c",c);
  switch(rank)
  { case RANK_1: c = '1'; break;
    case RANK_2: c = '2'; break;
    case RANK_3: c = '3'; break;
    case RANK_4: c = '4'; break;
    case RANK_5: c = '5'; break;
    case RANK_6: c = '6'; break;
    case RANK_7: c = '7'; break;
    case RANK_8: c = '8'; break;
  }
  printf("%c",c);

  file = File(m.to);
  rank = Rank(m.to);
  switch(file)
  { case FILE_A: c = 'a'; break;
    case FILE_B: c = 'b'; break;
    case FILE_C: c = 'c'; break;
    case FILE_D: c = 'd'; break;
    case FILE_E: c = 'e'; break;
    case FILE_F: c = 'f'; break;
    case FILE_G: c = 'g'; break;
    case FILE_H: c = 'h'; break;
    default: break;
  }
  printf("%c",c);
  switch(rank)
  { case RANK_1: c = '1'; break;
    case RANK_2: c = '2'; break;
    case RANK_3: c = '3'; break;
    case RANK_4: c = '4'; break;
    case RANK_5: c = '5'; break;
    case RANK_6: c = '6'; break;
    case RANK_7: c = '7'; break;
    case RANK_8: c = '8'; break;
  }
  printf("%c",c);
  switch(m.promoted)
  { case WQ:
    case BQ:
      printf("Q");
      break;
    case WR: 
    case BR:
      printf("R");
      break;
    case WB:
    case BB:
      printf("B");
      break;
    case WN:
    case BN:
      printf("N");
      break;
    default:	break;
  }
  //printf("\0");
  printf(" ");
}

bool parse_move(char *sInput, move_t *move)
{/// "e2e4" notation.
  
  int file,rank;
  char c = (char)tolower(*sInput);

  memset(move,0,sizeof(move_t));

  switch(c)
  { case 'a': file = FILE_A; break;
    case 'b': file = FILE_B; break;
    case 'c': file = FILE_C; break;
    case 'd': file = FILE_D; break;
    case 'e': file = FILE_E; break;
    case 'f': file = FILE_F; break;
    case 'g': file = FILE_G; break;
    case 'h': file = FILE_H; break;
    default: return false;break;
  }
  sInput++;
  switch(*sInput)
  { case '1': rank = RANK_1; break;
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
  move->from = (uint8)CalcSq(file, rank);
  c = (char)tolower(*sInput);
  switch(c)
  { case 'a': file = FILE_A; break;
    case 'b': file = FILE_B; break;
    case 'c': file = FILE_C; break;
    case 'd': file = FILE_D; break;
    case 'e': file = FILE_E; break;
    case 'f': file = FILE_F; break;
    case 'g': file = FILE_G; break;
    case 'h': file = FILE_H; break;
    default: return false;break;
  }
  sInput++;

  switch(*sInput)
  { case '1': rank = RANK_1; break;
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
  move->to = (uint8)CalcSq(file, rank);

  if(*sInput)
  { c = (char)tolower(*sInput);
    switch(c)
    { case 'q': move->promoted = (uint8)(Coloured(QUEEN ,pos->side)); break;
      case 'n': move->promoted = (uint8)(Coloured(KNIGHT ,pos->side)); break;
      case 'r': move->promoted = (uint8)(Coloured(ROOK ,pos->side)); break;
      case 'b': move->promoted = (uint8)(Coloured(BISHOP, pos->side)); break;
      case '\n': case ' ': break;
      default : return false; break;
    }
  }
  return true;
}

int pv_to_string(char *s, int depth)
{ 
  int i, file, rank;
  int len = 0, move_count = 0;
  uint32 pvline[MAXPLY];
  move_t m;
  
  pvline[0] = si.rootmove.p;
  m.p = si.rootmove.p;
  move_make(m);
  for(i = 1; i < MAXPLY; i++)
  { m.p = tt_get_hashmove();
    if(!m.p || !is_legal(m)) break;
    move_make(m);
    pvline[i] = m.p;
  }
  len = i;
  while(i--) move_unmake();
  
  //Avoid possible repetitions:
  if(len >= MAXPLY - 2) len = depth;

  //Step 3: Convert to string:
  for(i = 0; i < len; i++)
  { m.p = pvline[i];
    file = File(m.from);
    rank = Rank(m.from);
    switch(file)
    { case FILE_A: *s = 'a'; break;
      case FILE_B: *s = 'b'; break;
      case FILE_C: *s = 'c'; break;
      case FILE_D: *s = 'd'; break;
      case FILE_E: *s = 'e'; break;
      case FILE_F: *s = 'f'; break;
      case FILE_G: *s = 'g'; break;
      case FILE_H: *s = 'h'; break;
      default: break;
    }
    s++;
    switch(rank)
    { case RANK_1: *s = '1'; break;
      case RANK_2: *s = '2'; break;
      case RANK_3: *s = '3'; break;
      case RANK_4: *s = '4'; break;
      case RANK_5: *s = '5'; break;
      case RANK_6: *s = '6'; break;
      case RANK_7: *s = '7'; break;
      case RANK_8: *s = '8'; break;
    }
    s++;
    file = File(m.to);
    rank = Rank(m.to);
    switch(file)
    { case FILE_A: *s = 'a'; break;
      case FILE_B: *s = 'b'; break;
      case FILE_C: *s = 'c'; break;
      case FILE_D: *s = 'd'; break;
      case FILE_E: *s = 'e'; break;
      case FILE_F: *s = 'f'; break;
      case FILE_G: *s = 'g'; break;
      case FILE_H: *s = 'h'; break;
      default: break;
    }
    s++;
    switch(rank)
    { case RANK_1: *s = '1'; break;
      case RANK_2: *s = '2'; break;
      case RANK_3: *s = '3'; break;
      case RANK_4: *s = '4'; break;
      case RANK_5: *s = '5'; break;
      case RANK_6: *s = '6'; break;
      case RANK_7: *s = '7'; break;
      case RANK_8: *s = '8'; break;
    }
    s++;
    switch(GetType(m.promoted))
    { case QUEEN:  *s = 'Q'; s++; break;
      case ROOK:   *s = 'R'; s++; break;
      case BISHOP: *s = 'B'; s++; break;	
      case KNIGHT: *s = 'N'; s++; break;
      default: break;
    }
    *s = ' ';
    s++;
    move_count++;
  }
  *s = '\0'; //null terminating
  return move_count;
}


void print_info(int r,int d)
{
  int i,mate;
  float score;
  char str[32];
  int t;
  uint64 nps = 0;
  int seldepth = 0;
  char pvstring[4096];
  double hprm = 0;

  if(opt.mode == UCI_MODE)
  {
    i = pv_to_string(&pvstring[0] ,d);
    if(i > d) seldepth = i;
    t = ((get_time() - opt.startup_time));
    if(t) nps = (si.nodes / t) * 1000;

    hprm = ((double)si.num_hash_saves/(double)opt.tt_numentries) * 1000.0;

    if(abs(r) >= MATEVALUE - MAXPLY)
    { if(r>0) mate = (MATEVALUE - r + 1) / 2;
      else mate = -(MATEVALUE + r) / 2;
      printf("info depth %d seldepth %d score mate %d time %d nodes "llufmt" nps "llufmt" hashfull %.0f pv %s",d,seldepth, mate, t,si.nodes,nps,hprm,pvstring);
    }
    else printf("info depth %d seldepth %d score cp %d time %d nodes "llufmt" nps "llufmt" hashfull %.0f pv %s",d,seldepth, r, t,si.nodes,nps,hprm,pvstring);
    #ifdef GTB
    if(tbhits > 0) printf("tbhits %d", tbhits);
    #endif
    printf("\n");
  }
  else
  { //reversing the score if needed:
    //+ WHITE, - BLACK
    if(opt.comp_side == BLACK && r != 0)
      score = -(r / ((float)100));
    else
      score = (r / ((float)100));

    printf("%2d "llufmt,d,si.nodes); 
    //converting the nodes count
    sprintf(str,llufmt,si.nodes);

    //formatting the spaces betw. nodes and pv:
    if(score >= 0) i = (int)(16 - strlen(str));
    else i = (int)(15 - strlen(str));
    while(i){
      printf(" ");
      i--;
    }
    printf(" %.2f ",score);
    pv_to_string(&pvstring[0],d);
    printf("%s", pvstring);
    printf("\n");
	}
}

void update_info()
{
  uint64 nps = 0;
  int t;
  double hprm = 0;
  if(opt.mode == UCI_MODE)
  { t = get_time() - opt.startup_time;
    if(t) nps = (si.nodes / t) * 1000;
    hprm = ((double)si.num_hash_saves/(double)opt.tt_numentries) * 1000.0;
    printf("info time %d nodes "llufmt" nps "llufmt" hashfull %.0f\n",t,si.nodes,nps,hprm);
  }
}


#ifndef _WIN32
#include <sys/time.h>
int poll()
{
  fd_set readfds;
  struct timeval  timeout;

  FD_ZERO(&readfds);
  FD_SET(fileno(stdin), &readfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  select(16, &readfds, 0, 0, &timeout);
  return (FD_ISSET(fileno(stdin), &readfds));
}
#else
#include <windows.h>
#include <conio.h>
int poll()
{	
  DWORD dwData,i;
  INPUT_RECORD inRec[8];
  static HANDLE hConsoleHandle;
  static bool poll_initialized = false;
  static bool pipe = true;

  if(!poll_initialized)
  { poll_initialized = true;
    hConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
    if(GetConsoleMode(hConsoleHandle, &dwData)) 
      pipe = false;
  }
  if(pipe)
  { if(PeekNamedPipe(hConsoleHandle, 0, 0, 0, &dwData, 0))
      return dwData;
  }
  else 
  { PeekConsoleInput(hConsoleHandle, &inRec[0], 8, &dwData);
    for(i = 0; i < dwData; i++)
      if(inRec[i].EventType & KEY_EVENT)
        return dwData;
  }
  return (pipe);
}
#endif

void check_for_poll()
{
  char input[256];

  if(poll())
  { if(!fgets(input,sizeof(input), stdin)) strcpy(input, "quit\n");
    if(opt.mode == UCI_MODE)
    { if(!strncmp(input, "quit", 4)) uci_exit();
      else if(!strncmp(input, "stop", 4))
        si.stop_search = true;
      else if(!strncmp(input, "isready",7)) printf("readyok\n");
      else if(!strncmp(input, "ponderhit",9))
      {////resets the clock and continue:
        opt.analyze = false;
        if(opt.rtp)
        { opt.startup_time = get_time(); 
          si.nodes = 0;
        }
      }
    }
  }
}

void suppress_buffering()
{
  #ifndef _WIN32
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  #else
  HANDLE hConsoleHandle;
  DWORD dwData;
  hConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
  if(!GetConsoleMode(hConsoleHandle, &dwData)) 
  { setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
  }
  #endif
}

char piece_to_char(int p)
{
  char c;
  switch(p)
  { case WP: c = 'P'; break;
    case BP: c = 'p'; break;
    case WK: c = 'K'; break;
    case BK: c = 'k'; break;
    case WQ: c = 'Q'; break;
    case BQ: c = 'q'; break;
    case WR: c = 'R'; break;
    case BR: c = 'r'; break;
    case WB: c = 'B'; break;
    case BB: c = 'b'; break;
    case WN: c = 'N'; break;
    case BN: c = 'n'; break;
    default: c =  0 ; break;
  }
  return c;
}

void board_display()
{
  int i,j,k;
  char c;
  
  printf("\n\n");
  for(i = RANK_8; i >= RANK_1; i--)
  { printf("   ");
    for(k = 0; k < 8; k++) printf("----");
    printf("-\n");
    printf(" %d ",i + 1); 
    printf("|");
    for(j = FILE_A; j <= FILE_H; j++)
    { c = piece_to_char(pos->square[CalcSq(j, i)]);
      if(c == 0) c = ' ';
      printf(" %c |", c);
      
    }
    printf("\n");
  }
  printf("   ");
  for(k = 0; k < 8; k++) printf("----");
  printf("-\n    ");
  for(k = 0; k < 8; k++) printf(" %c  ", 'A' + k);
  printf("\n\n");
}


void move_to_str(char *s, move_t m)
{ 
  int file, rank;
  
  if(m.p == 0)
  { s[0] = s[1] = s[2] = s[3] = '0';
    s[4] = '\0';
    return;
  }
   
  file = File(m.from);
  rank = Rank(m.from);
  switch(file)
  {
    case FILE_A: *s = 'a'; break;
    case FILE_B: *s = 'b'; break;
    case FILE_C: *s = 'c'; break;
    case FILE_D: *s = 'd'; break;
    case FILE_E: *s = 'e'; break;
    case FILE_F: *s = 'f'; break;
    case FILE_G: *s = 'g'; break;
    case FILE_H: *s = 'h'; break;
    default: break;
  }
  s++;
  switch(rank)
  {
    case RANK_1: *s = '1'; break;
    case RANK_2: *s = '2'; break;
    case RANK_3: *s = '3'; break;
    case RANK_4: *s = '4'; break;
    case RANK_5: *s = '5'; break;
    case RANK_6: *s = '6'; break;
    case RANK_7: *s = '7'; break;
    case RANK_8: *s = '8'; break;
  }
  s++;
  file = File(m.to);
  rank = Rank(m.to);
  switch(file)
  {
    case FILE_A: *s = 'a'; break;
    case FILE_B: *s = 'b'; break;
    case FILE_C: *s = 'c'; break;
    case FILE_D: *s = 'd'; break;
    case FILE_E: *s = 'e'; break;
    case FILE_F: *s = 'f'; break;
    case FILE_G: *s = 'g'; break;
    case FILE_H: *s = 'h'; break;
    default: break;
  }
  s++;
  switch(rank)
  {
    case RANK_1: *s = '1'; break;
    case RANK_2: *s = '2'; break;
    case RANK_3: *s = '3'; break;
    case RANK_4: *s = '4'; break;
    case RANK_5: *s = '5'; break;
    case RANK_6: *s = '6'; break;
    case RANK_7: *s = '7'; break;
    case RANK_8: *s = '8'; break;
  }
  s++;
  switch(GetType(m.promoted))
  {
    case QUEEN:  *s = 'Q'; s++; break;
    case ROOK:   *s = 'R'; s++; break;
    case BISHOP: *s = 'B'; s++; break;	
    case KNIGHT: *s = 'N'; s++; break;
    default: break;
  }
  *s = '\0'; //null terminating
}

