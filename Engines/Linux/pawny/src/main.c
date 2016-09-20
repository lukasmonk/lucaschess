/*--------------------------------------------------------------------------
    Pawny 1.0, chess engine (source code).
    Copyright (C) 2009 - 2013 by Mincho Georgiev.
    
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
#include "inline.h"

#ifndef POPCNT
void init_bittable()
{ 
  int i;
  unsigned int w;
  for(i = 0; i < 65535; i++)
  { w = (unsigned int)(i & 0xFFFFFFFF);
    w-= (w >> 1) & 0x55555555;
    w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    w = (w + (w >> 4)) & 0x0F0F0F0F;
    bittable[i] = (w * 0x01010101) >> 24;
  }
}
#endif
  
void init()
{	
  #ifndef POPCNT
  init_bittable();
  #endif
  init_movegen();
  position_set(INITIAL_POSITION);
  suppress_buffering();
  tt_init(DEFAULT_TT_SIZE);
  pnt_init();

  //load defaults:
  opt.mode = CONSOLE_MODE;
  opt.analyze = false;
  opt.time_high = 50000;//50 sec. default max time/move
  opt.time_low  = 30000;
  opt.aspiration = DEFAULT_ASPIRATION_WINDOW;
  opt.rtp = false;
  memset(&si,0,sizeof(searchinfo_t));
  init_material();  
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
  for(;;)
  { p = fgets(sInput,sizeof(sInput),stdin);
    //null terminating:
    while(*p != '\n') p++;
    *p = '\0';
    //parse:
    if(!parse_input(sInput))
    { 
      if(parse_move(sInput, &move))
      { if(!make_if_legal(move))
        { printf("Illegal move\n\n");
          continue;
        }
        opt.comp_side = pos->side;
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
  char strbuff[256];
  bool result = false;
  int x;
	
  if(*sInput == '\0') return true;

  else if(!strcmp(sInput, "uci")) uci(pos);
  else if(!strcmp(sInput,"quit")) exit(0);
	
  else if(!strcmp(sInput,"undo"))
  { move_unmake(pos);
    result = true;
  }
  else if(!strcmp(sInput,"new"))
  { position_set(INITIAL_POSITION);
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
  else if(!strcmp(sInput,"getfen"))
	{ position_get(&strbuff[0]);
    printf("\n");
    printf("%s",strbuff);
    printf("\n\n");
    result = true;
  }
  else if(strstr(sInput,"setboard"))
  { sInput += sizeof("setboard");
    while(isspace(*sInput)) sInput++;
    if(!position_set(sInput))
      printf ("Illegal position (FEN) !\n");
    result = true;
  }
  else if(!strncmp(sInput, "epdtest",7))
  { sInput += sizeof("epdtest");
    while(isspace(*sInput)) sInput++;
    if(!test_epd(sInput)) printf ("EPD error!\n");
    result = true;
  }
  else if(!strncmp(sInput, "perft",5))
  { sInput += sizeof("perft");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      perftest(x);
    }
    result = true;
  }
  else if(!strcmp(sInput,"eval"))
  { int r; float score;
    r = eval();
    if(opt.comp_side == BLACK && r != 0)
      score = -(r / ((float)100));
    else
      score = (r / ((float)100));
    printf(" %.2f \n",score);
    result = true;
  }
		
  else if(!strcmp(sInput,"go"))
  { opt.comp_side = pos->side;
    think(pos);
    result = true;
  }
	
  else if(strstr(sInput,"sd"))
  { sInput += sizeof("sd");
    while(isspace(*sInput)) sInput++;
    if(isdigit(*sInput)) 
    { x = atoi(sInput);
      opt.max_depth = x;
      opt.analyze = true;
    }
    result = true;
  }
	
  return result;
}
