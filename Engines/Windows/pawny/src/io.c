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


void print_info(int r,int d)
{
  int i,mate;
  float score;
  char str[32];
  int t;
  uint64 nps = 0;
  int seldepth = 0;
  char pvstring[1024];
  double hprm = 0;

  if(opt->mode == UCI_MODE)
  {
    i = pv_to_string(&pvstring[0],d);
    if(i > d) seldepth = i;
    t = ((get_time() - opt->startup_time));
    if(t) nps = (si->nodes / t) * 1000;
    
    hprm = ((double)si->num_hash_saves/(double)opt->tt_numentries) * 1000.0;

    if(abs(r) >= MATE_VALUE - MAX_PLY)
    { if(r>0) mate = (MATE_VALUE - r + 1) / 2;
      else mate = -(MATE_VALUE + r) / 2;
      printf("info depth %d seldepth %d score mate %d time %d nodes "llufmt" nps "llufmt" hashfull %.0f pv %s",d,seldepth, mate, t,si->nodes,nps,hprm,pvstring);
    }
    else printf("info depth %d seldepth %d score cp %d time %d nodes "llufmt" nps "llufmt" hashfull %.0f pv %s",d,seldepth, r, t,si->nodes,nps,hprm,pvstring);
    #ifdef GTB
    if(tbhits > 0) printf("tbhits %d", tbhits);
    #endif
    printf("\n");
  }
  else
  { //reversing the score if needed:
    //+ WHITE, - BLACK
    if(opt->comp_side == BLACK && r != 0)
      score = -(r / ((float)100));
    else
      score = (r / ((float)100));
    
    printf("%2d "llufmt,d,si->nodes); 
    //converting the nodes count
    sprintf(str,llufmt,si->nodes);

    //formatting the spaces betw. nodes and pv:
    if(score >= 0) i = (int)(11 - strlen(str));
    else i = (int)(10 - strlen(str));
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
  
  if(opt->mode == UCI_MODE)
  { t = get_time() - opt->startup_time;
    if(t) nps = (si->nodes / t) * 1000;
    hprm = ((double)si->num_hash_saves/(double)opt->tt_numentries) * 1000.0;
    printf("info time %d nodes "llufmt" nps "llufmt" hashfull %.0f\n",t,si->nodes,nps,hprm);
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
  { if (!fgets(input,sizeof(input), stdin)) strcpy(input, "quit\n");
    if(opt->mode == UCI_MODE)
    { if (!strncmp(input, "quit", 4)) uci_exit();
      else if (!strncmp(input, "stop", 4))
        si->stop_search = true;
      else if(!strncmp(input, "isready",7)) printf("readyok\n");
      else if(!strncmp(input, "ponderhit",9))
      { //resets the clock and continue:
        opt->analyze = false;
        if(opt->rtp)
        { opt->startup_time = get_time(); 
          si->nodes = 0;
        }
      }
    }
  }
}

move_data_t pv_backup[MAX_PLY];
static void collect(int *psize)
{
  move_t m;
  uint32 hm;
  
  if(*psize >= MAX_PLY) return;
  hm = tt_get_hashmove();
  if(hm)
  { m.p = hm;
    if(make_if_legal(m))
    { pv_backup[*psize].p = hm;
      *psize += 1;
      collect(psize);
      move_undo();
    }
  }
}

static int pv_collect()
{
  move_t m;
  int pvlen = 1;
  
  m.p = si->rootmove.p;
  if(!make_if_legal(m)) return 1;
  pv_backup[0].p = m.p;
  collect(&pvlen);
  move_undo();
  return pvlen;
}

int pv_to_string(char *s,int depth)
{//takes tha addr of buffer as argument 
//and returns the pv string length in moves count.
  int i,file,rank;
  move_data_t m;
  int len,move_count = 0;

  len = pv_collect();
  
  //check for redundancy repetitions:
  if(len > MAX_PLY-1) len = depth;
  
  for(i = 0; i < len; i++)
  {
    m.p = pv_backup[i].p;
    
    file = calc_file(m.from);
    rank = calc_rank(m.from);
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
    file = calc_file(m.to);
    rank = calc_rank(m.to);
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
    *s = ' ';
    s++;
    move_count++;
  }
  *s = '\0'; //null terminating
  
  return move_count;
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
