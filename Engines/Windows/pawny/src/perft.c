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
#include "bits.h"

#define PHT_NUM_ENTRY 1000000//~25MB pht size (1 000 000 entries by 24 bytes of size)
#define PHT_SIZE (PHT_NUM_ENTRY * sizeof(ph_entry_t))
#define HINDEX ((board->hash % PHT_NUM_ENTRY))

#define EVASIONS
#define BITS

static int pawnhits;
static int pawncollisions;

typedef struct ph_entry_t
{ int depth;
  uint64 nodes;
  uint64 key;
  uint64 pkey;
}ph_entry_t;

//global table pointer:
ph_entry_t *pht;


void pht_init()
{ pht = (ph_entry_t *)malloc(PHT_SIZE);
  if(!pht)
  { printf("not enough memory.\n\n");
    return;
  }
  memset(pht, 0, (PHT_SIZE));
}

void pht_free()
{ free(pht);
}

uint64 probe_hash(int depth)
{
  ph_entry_t *p = &pht[HINDEX];
  if(p->key == board->hash)
  { if(p->pkey == board->phash) pawnhits++;
    else pawncollisions++;
    if(p->depth == depth) return (p->nodes);
  }
  return 0;
}

void record_hash(int depth, uint64 nodes)
{ ph_entry_t *p = &pht[HINDEX];
  p->key = board->hash;
  p->pkey = board->phash;
  p->nodes = nodes;
  p->depth = depth;
}


uint64 perft(int depth)
{ 
  int i,move_count;
  move_t ms[MOVE_STACK];
  uint64 nodes;
  uint64 val;
  #ifdef EVASIONS
  char strbuff[256];
  move_t ms_test[MOVE_STACK];
  #endif
  #ifdef BITS
  bitboard_t bb;
  #endif
  
  if(depth == 0) return 1;
  if((val = probe_hash(depth)) != 0) 
    return val;

  nodes = 0;
  val = 0;
    
  #ifdef BITS
  bb = 0;
  for(i = PLS(WP); i <= H8; i = PLN(i))
    bitset(&bb, rsz[i]);
  if(bb != board->bb_pawns[W])
    printf("pawn_error\n");
  bb = 0;
  for(i = PLS(BP); i <= H8; i = PLN(i))
    bitset(&bb, rsz[i]);
  if(bb != board->bb_pawns[B])
    printf("pawn_error\n");
  #endif
  
  #ifdef EVASIONS
  if(is_in_check(board->side))
  { move_count = move_gen_evasions(&ms[0]);
    if(move_count != move_gen_legal(&ms_test[0]))
    { board_to_fen(&strbuff[0]);
      printf("error: \n %s \n",strbuff);
    }
  }
  else
  { move_count = move_gen(&ms[0]);
  }
  #else
  move_count = move_gen(&ms[0]);
  #endif
    
  for(i = 0; i < move_count; i++)
  { if(!move_make(ms[i])) continue;
    if(depth == 1)
      nodes++;
    else
      nodes += perft(depth - 1);

    move_undo();
  }
  record_hash(depth,nodes);
  return (nodes);
}

void test_divide(int depth)
{
  int i,move_count;
  move_t ms[MOVE_STACK];
  uint64 nodes;
  uint64 total_nodes;
  int legal_moves;
  #ifdef EVASIONS
  char strbuff[256];
  move_t ms_test[MOVE_STACK];
  #endif
  
  if(!depth) return;
  nodes = 0;
  total_nodes = 0;
  legal_moves = 0;
  
  pht_init();
  depth -= 1;
  timer_start();
    
  #ifdef EVASIONS
  if(is_in_check(board->side))
  { move_count = move_gen_evasions(&ms[0]);
    if(move_count != move_gen_legal(&ms_test[0]))
    { board_to_fen(&strbuff[0]);
      printf("error: \n %s \n",strbuff);
    }
  }
  else
  { move_count = move_gen(&ms[0]);
  }
  #else
  move_count = move_gen(&ms[0]);
  #endif
    
  for(i = 0; i < move_count; i++)
  { if(!move_make(ms[i])) continue;
    nodes = perft(depth);
    print_move(ms[i].p);
    legal_moves++;
    printf("%I64u",nodes);
    printf("\n");
    move_undo();
    total_nodes += nodes;
  }
  printf("\nNodes: %I64u",total_nodes);
  printf("\nMoves: %d",legal_moves);
  printf("\n\n");
  timer_stop();
  pht_free();
}


void test_perft(int depth)
{//itterative perft calling and output:
  int i;
  uint64 nodes;
    
  pht_init();		
  printf("\n");
  printf("-----------------------\n");
  printf(" depth   nodes \n");
  printf("-----------------------\n");
  depth += 1;
  for(i = 1; i < depth; i++)
  { nodes = perft(i);
    printf("%6d   %I64u", i,nodes);
    printf("\n");
  }
  printf("-----------------------\n");
  pht_free();
}


bool test_epd(char *filename)
{
  FILE *f;
  char c,*p;
  char line[255];
  char buff[255];
  uint64 nodes,result;
  int x,depth,position,errors,sum;

  f = fopen(filename, "r");
  if(f == NULL) return false;
  pht_init();
  timer_start();
  sum = 0;position = 0;result = 0;
  while(fgets(&line[0], sizeof(line), f))
  {				
    x = 0; p = &line[0];
    while(*p != ';'){	
      buff[x] = *p;
      p++; x++;
    }
    buff[x] = '\0';
    position++;
      
    if(!board_from_fen(&buff[0])){
      printf ("position %d - FEN error!\n\n", position);
      continue;
    }
    printf("\nposition %d\n",position);
    printf("----------------------\n");
    printf("%s\n",&buff[0]);
    printf("----------------------\n");
    
    depth = 0; errors = 0;
    while((*p != '\n') && (*p != 0))
    {
      if(*p == 'D'){
        depth++;
        sscanf(p,"%s %I64u",&c,&result);
        printf("depth %d: ",depth);
        nodes = perft(depth);
        if(nodes != result){
          printf("error\n");
          errors++;
        }
        else printf("done\n");
      }
      p++;
    }
    printf("----------------------\n");
    printf("unmatched: %d \n\n", errors);
    sum+=errors;
  }
  printf("----------------------\nend_of_file.\n");
  printf("total mismatched depths: %d\n",sum);
  timer_stop();
  fclose(f);
  pht_free();
  return true;
}


void test_pawn_divide(int depth)
{
  int i,move_count;
  move_t ms[MOVE_STACK];
  uint64 nodes = 0;
  uint64 total_nodes = 0;
  int legal_moves = 0;
  
  if(!depth) return;
  pawnhits = 0;
  pawncollisions = 0;
  pht_init();
  depth -= 1;
  timer_start();
  move_count = move_gen(&ms[0]);
  for(i = 0; i < move_count; i++)
  {
    if(!move_make(ms[i])) continue;
    nodes = perft(depth);
    print_move(ms[i].p);
    legal_moves++;
    printf("%I64u",nodes);
    printf("\n");
    move_undo();
    total_nodes += nodes;
  }
  printf("\nNodes: %I64u",total_nodes);
  printf("\nMoves: %d",legal_moves);
  printf("\nPawnHits: %d",pawnhits);
  printf("\nPawnCollisions: %d",pawncollisions);
  printf("\n\n");
  timer_stop();
  pht_free();
}


