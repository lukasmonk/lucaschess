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
#ifdef GTB
#include "gtb-probe.h"
#include "inline.h"

#define WDL_FRACTION 0
#define GTB_VERBOSE 0

bool gtb_uci;
bool gtb_ok;
int gtb_max_men;
int gtb_cachesize;
char gtb_path[256];
int gtb_dec_scheme;
int tbhits;
static const char **gtb_paths;

void gtb_clear()
{ tbcache_done();
  tb_done();
  tbpaths_done(gtb_paths);
}

void gtb_init()
{
  int av;
  
  //already initialized
  if(gtb_ok) gtb_clear();
  
  if(gtb_cachesize == 0 || gtb_dec_scheme < 0 
  || gtb_dec_scheme > 9 || gtb_path[0] == 0)
  { gtb_ok == false;
    return;
  }
  
  gtb_paths = tbpaths_init();
  gtb_paths = tbpaths_add(gtb_paths, &gtb_path[0]);
  tb_init(GTB_VERBOSE, gtb_dec_scheme, gtb_paths);
  tbcache_init(gtb_cachesize, WDL_FRACTION);
  tbstats_reset();
  gtb_ok = (tb_is_initialized() && tbcache_is_on());
  if(!gtb_ok) return;
  
  av = tb_availability();
  if(av & 16) gtb_max_men = 5;
  else if(av & 8) gtb_max_men = 4;
  else if(av & 2) gtb_max_men = 3;
  else
  { gtb_max_men = 0;
    gtb_ok = false;
    gtb_clear();
  }
}

bool gtb_probe(int *value)
{
  int sq, w, b, success;
  unsigned int  ws[17], bs[17];
  unsigned char wp[17], bp[17];
  unsigned pliestomate;	
  unsigned info = tb_UNKNOWN;
  unsigned int tb_castling = 0;
  unsigned int ep;
  bitboard_t t;
  
  ws[0] = bitscanf(pos->occ[WK]);
  wp[0] = tb_KING;
  bs[0] = bitscanf(pos->occ[BK]);
  bp[0] = tb_KING;
  w = b = 1;
  
  t = pos->occ[WP];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_PAWN;
    w++;
  }
  
  t = pos->occ[WN];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_KNIGHT;
    w++;
  }
  
  t = pos->occ[WB];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_BISHOP;
    w++;
  }
  
  t = pos->occ[WR];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_ROOK;
    w++;
  }
  
  t = pos->occ[WQ];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_QUEEN;
    w++;
  }
  
  t = pos->occ[BP];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_PAWN;
    b++;
  }
  
  t = pos->occ[BN];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_KNIGHT;
    b++;
  }
  
  t = pos->occ[BB];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_BISHOP;
    b++;
  }
  
  t = pos->occ[BR];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_ROOK;
    b++;
  }
  
  t = pos->occ[BQ];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_QUEEN;
    b++;
  }
  
  ws[w] = tb_NOSQUARE;
  wp[w] = tb_NOPIECE;
  bs[b] = tb_NOSQUARE;
  bp[b] = tb_NOPIECE;
  
  ep = pos->ep ? (pos->ep) : (tb_NOSQUARE);
  
  if(pos->castle)
  { tb_castling  = ((1 && (pos->castle & WHITE_OO))  * tb_WOO);
    tb_castling |= ((1 && (pos->castle & WHITE_OOO)) * tb_WOOO);
    tb_castling |= ((1 && (pos->castle & BLACK_OO))  * tb_BOO);
    tb_castling |= ((1 && (pos->castle & BLACK_OOO)) * tb_BOOO);
  }
  
  if((opt.analyze) || ((get_time() - opt.startup_time) < opt.time_low/2 && opt.time_low > 500))
    success = tb_probe_hard(pos->side, ep, tb_castling, ws, bs, wp, bp, &info, &pliestomate);
  else success = tb_probe_soft(pos->side, ep, tb_castling, ws, bs, wp, bp, &info, &pliestomate);
  if(success)
  { tbhits++;
    if(info == tb_DRAW) *value = 0;
    else if((info == tb_WMATE && pos->side == W)
          ||(info == tb_BMATE && pos->side == B))
      *value = (MATEVALUE-pliestomate)-1;
    else if((info == tb_WMATE && pos->side == B)
          ||(info == tb_BMATE && pos->side == W))
      *value = (-MATEVALUE+pliestomate)+1;
    return true;
  }
  return false;
}

bool gtb_root_probe(int *value)
{
  int sq, w, b;
  unsigned int  ws[17], bs[17];
  unsigned char wp[17], bp[17];
  unsigned pliestomate;	
  unsigned info = tb_UNKNOWN;
  unsigned int tb_castling = 0;
  unsigned int ep;
  bitboard_t t;
  
  ws[0] = bitscanf(pos->occ[WK]);
  wp[0] = tb_KING;
  bs[0] = bitscanf(pos->occ[BK]);
  bp[0] = tb_KING;
  w = b = 1;
  
  t = pos->occ[WP];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_PAWN;
    w++;
  }
  
  t = pos->occ[WN];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_KNIGHT;
    w++;
  }
  
  t = pos->occ[WB];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_BISHOP;
    w++;
  }
  
  t = pos->occ[WR];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_ROOK;
    w++;
  }
  
  t = pos->occ[WQ];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    ws[w] = sq;
    wp[w] = tb_QUEEN;
    w++;
  }
  
  t = pos->occ[BP];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_PAWN;
    b++;
  }
  
  t = pos->occ[BN];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_KNIGHT;
    b++;
  }
  
  t = pos->occ[BB];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_BISHOP;
    b++;
  }
  
  t = pos->occ[BR];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_ROOK;
    b++;
  }
  
  t = pos->occ[BQ];
  while(t)
  { sq = bitscanf(t);
    bitclear(t, sq);
    bs[b] = sq;
    bp[b] = tb_QUEEN;
    b++;
  }
  
  ws[w] = tb_NOSQUARE;
  wp[w] = tb_NOPIECE;
  bs[b] = tb_NOSQUARE;
  bp[b] = tb_NOPIECE;
  
  ep = pos->ep ? (pos->ep) : (tb_NOSQUARE);
  
  if(pos->castle)
  { tb_castling  = ((1 && (pos->castle & WHITE_OO))  * tb_WOO);
    tb_castling |= ((1 && (pos->castle & WHITE_OOO)) * tb_WOOO);
    tb_castling |= ((1 && (pos->castle & BLACK_OO))  * tb_BOO);
    tb_castling |= ((1 && (pos->castle & BLACK_OOO)) * tb_BOOO);
  }
    if(tb_probe_hard(pos->side, ep, tb_castling, ws, bs, wp, bp, &info, &pliestomate))
  { tbhits++;
    if(info == tb_DRAW) *value = 0;
    else if((info == tb_WMATE && pos->side == W)
          ||(info == tb_BMATE && pos->side == B))
      *value = (MATEVALUE-pliestomate)-1;
    else if((info == tb_WMATE && pos->side == B)
          ||(info == tb_BMATE && pos->side == W))
      *value = (-MATEVALUE+pliestomate)+1;
    return true;
  }
  return false;
}

int gtb_men()
{
  return(pos->pcount[WP] + 
         pos->pcount[WN] + 
         pos->pcount[WB] + 
         pos->pcount[WR] + 
         pos->pcount[WQ] + 
         pos->pcount[BP] + 
         pos->pcount[BN] + 
         pos->pcount[BB] + 
         pos->pcount[BR] + 
         pos->pcount[BQ] + 2);
}

#endif