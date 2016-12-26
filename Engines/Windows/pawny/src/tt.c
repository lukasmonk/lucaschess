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
#include <malloc.h>

#define BUCKET_SIZE 4
#define MAX_AGE 15
#define MAX_DRAFT 256
#define ALIGNMENT (sizeof(tt_t) * BUCKET_SIZE)

static int age;

void tt_free()  {aligned_free((void *)tt); tt = 0;}
void tt_clear() {aligned_wipe_out((void *)tt,(size_t)(opt->tt_size),(size_t)ALIGNMENT);}

int tt_init(int MB)
{///min size - 8MB, max - 1024:
  tt_free();
  if(MB < 8) MB = 8; if(MB > 1024) MB = 1024;
  opt->tt_size = MB*1024*1024;
  tt = (tt_t *)aligned_malloc((size_t)opt->tt_size, ALIGNMENT);
  if(!tt) return false;
  opt->tt_numentries = ((opt->tt_size / ALIGNMENT) - 1) * BUCKET_SIZE;
  tt_clear();
  age = 1;
  return true;
}

uint8 tt_probe(int depth, int *value, int beta, uint32 *h_move, uint8 *null_ok)
{
  tt_t *h_entry; int i;
  
  h_entry = tt + (board->hash & opt->tt_numentries);
  
  for(i = 0; i < BUCKET_SIZE; i++)
  { if((h_entry+i)->key == board->hash)
      break;
  }
  if(i < BUCKET_SIZE)
  { h_entry += i;
    //'is_null_move_relevant' test:
    if((h_entry->flag & 0x0F) == TT_ALPHA 
    &&  h_entry->value < beta
    && ((depth - 3 - 1) <= h_entry->depth))
      *null_ok = false;
    
    *h_move = h_entry->move;
    if(h_entry->depth >= depth)
    { *value = h_entry->value;
      return (h_entry->flag & 0x0F);
    }
  }
  return TT_UNKNOWN;
}

void tt_save(int depth, int value, uint8 flag, uint32 h_move)
{
  int i,draft_replace;
  tt_t *h_entry, *h_replace = 0;
  
  h_entry = tt + (board->hash & opt->tt_numentries);
  
  draft_replace = MAX_DRAFT; //'firstentry' hack
  
  for(i = 0; i < BUCKET_SIZE; i++)
  { if((h_entry+i)->key == board->hash)
    { //direct hit -> unconditional replacement:
      h_entry += i;
      si->num_hash_saves++;
      h_entry->value = (sint16)value;
      h_entry->depth = (uint8)depth;
      h_entry->flag  = (uint8)(age << 4) | flag;
      if(h_move) h_entry->move = h_move;
      return;
    }

    //looking for 'another search' entry
    //that holds the lowest draft so far.
    if(((h_entry+i)->flag>>4) != age
    && (h_entry+i)->depth < draft_replace)
    { h_replace = h_entry + i;
      draft_replace = (h_entry + i)->depth;
    }
  }
  
  //still no entry to replace, so looking
  //for the entry with the lowest draft, regardless
  //of the hash generation:
  if(!h_replace)
  { for(i = 0; i < BUCKET_SIZE; i++)
    { if((h_entry+i)->depth < draft_replace)
      { h_replace = h_entry + i;
        draft_replace = (h_entry + i)->depth;
      }
    }
  }
      
  //replacing the 'draft|age prefered' entry:
  si->num_hash_saves++;
  h_replace->key   = board->hash;
  h_replace->value = (sint16)value;
  h_replace->depth = (uint8)depth;
  h_replace->flag  = (uint8)(age << 4) | flag;
  if(h_move) h_replace->move = h_move;
}

void tt_check()
{
  if(age > MAX_AGE) age = 1;
  else age++;
}	

uint32 tt_get_hashmove()
{///getting the hash move for the current pos.
  int i;
  
  tt_t *h_entry = tt + (board->hash & opt->tt_numentries);
  
  for(i = 0; i < BUCKET_SIZE; i++)
  { if((h_entry+i)->key == board->hash)
      return (h_entry+i)->move;
  }
  return 0;
}

int tt_retrieve_ponder(move_t *m)
{///retrieving the next move from tt
  move_make(si->rootmove);
  m->p = tt_get_hashmove();
  if(!m->p)
  { move_undo();
    return false;
  }
  move_undo();
  return true;
}

