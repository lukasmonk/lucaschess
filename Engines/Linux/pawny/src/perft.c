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

#define HASHING
#define PHT_NUM_ENTRIES 1000000
#define PHT_SIZE (PHT_NUM_ENTRIES * sizeof(pht_entry_t))
#define HINDEX (sig % PHT_NUM_ENTRIES)

#ifdef HASHING
typedef struct
{ int depth;
  uint64 nodes;
  uint64 key;
}pht_entry_t;

pht_entry_t *pht;

uint64 probe_hash(uint64 sig, int depth)
{ pht_entry_t *p = &pht[HINDEX];
  if(p->key == sig && p->depth == depth) 
    return (p->nodes);
  return 0;
}

void store_hash(uint64 sig, int depth, uint64 nodes)
{ pht_entry_t *p = &pht[HINDEX];
  p->key = sig;
  p->nodes = nodes;
  p->depth = depth;
}
#endif

uint64 perft(int depth)
{
  int i,move_count;
  move_t ms[MSSIZE];
  uint64 nodes = 0;

  if(depth == 0) return 1;
  #ifdef HASHING
  if((nodes = probe_hash(pos->hash, depth)) != 0) 
    return nodes;
  #endif
  move_count = move_gen(&ms[0]);
  for(i = 0; i < move_count; i++)
  { move_make(ms[i]);
    if(depth == 1) nodes++;
    else nodes += perft(depth - 1);
    move_unmake(pos);
  }
  #ifdef HASHING
  store_hash(pos->hash, depth, nodes);
  #endif
  return nodes;
}

void test_divide(int depth)
{
  int i,move_count;
  move_t ms[MSSIZE];
  uint64 nodes;
  uint64 total_nodes = 0;
  int legal_moves = 0;

  if(!pos || depth <= 0) return;
  #ifdef HASHING
  pht = (pht_entry_t *)aligned_malloc(PHT_SIZE, 64);
  aligned_wipe_out(pht, PHT_SIZE, 64);
  #endif
  timer_start();
  move_count = move_gen(&ms[0]);
  for(i = 0; i < move_count; i++)
  { move_make(ms[i]);
    nodes = perft(depth-1);
    total_nodes += nodes;
    legal_moves++;
    print_move(ms[i].p);
    printf("%I64u\n",nodes);
    move_unmake(pos);
  }
  printf("\nNodes: %I64u",total_nodes);
  printf("\nMoves: %d",legal_moves);
  printf("\n\n");
  timer_stop();
  #ifdef HASHING
  aligned_free(pht);
  #endif
}

bool test_epd(char *filename)
{
  FILE *f;
  char c[8], *p;
  char line[255];
  char buff[255];
  uint64 nodes,result;
  int x,depth,position,errors,sum;

  #ifdef HASHING
  pht = (pht_entry_t *)aligned_malloc(PHT_SIZE, 64);
  aligned_wipe_out(pht, PHT_SIZE, 64);
  #endif
  f = fopen(filename, "r");
  if(f == NULL) return false;

  timer_start();
  sum = 0;position = 0;result = 0;
  while(fgets(&line[0], sizeof(line), f))
  { x = 0; 
    p = &line[0];
    while(*p != ';')
    { buff[x] = *p;
      p++; x++;
    }
    buff[x] = '\0';
    position++;

    if(!position_set(&buff[0]))
    { printf ("position %d - FEN error!\n\n", position);
      continue;
    }
    printf("\nposition %d\n",position);
    printf("----------------------\n");
    printf("%s\n",&buff[0]);
    printf("----------------------\n");

    depth = 0; errors = 0;
    while((*p != '\n') && (*p != 0))
    { if(*p == 'D')
      { depth++;
        sscanf(p,"%s %I64u",&c[0], &result);
        printf("depth %d: ",depth);
        nodes = perft(depth);
        if(nodes != result)
        { printf("error\n");
          errors++;
        }
        else printf("done\n");
      }
      p++;
    }
    printf("----------------------\n");
    printf("unmatched: %d\n\n", errors);
    sum+=errors;
  }
  printf("----------------------\nend_of_file.\n");
  printf("total mismatched depths: %d\n",sum);
  timer_stop();
  fclose(f);
  #ifdef HASHING
  aligned_free(pht);
  #endif
  return true;
}

void perftest(int depth)
{
  int i;
  uint64 nodes;

  if(!pos || depth <= 0) return;
  #ifdef HASHING
  pht = (pht_entry_t *)aligned_malloc(PHT_SIZE, 64);
  aligned_wipe_out(pht, PHT_SIZE, 64);
  #endif
  printf("\n-----------------------\n");
  printf(" depth   nodes \n");
  printf("-----------------------\n");
  for(i = 1; i <= depth; i++)
  { nodes = perft(i);
    printf("%6d   %I64u\n", i, nodes);
  }
  printf("-----------------------\n");
  #ifdef HASHING
  aligned_free(pht);
  #endif
}
