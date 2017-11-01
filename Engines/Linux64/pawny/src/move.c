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
#include "inline.h"

static void move_make_w(move_t m)
{ int sply = pos->sply;
  uint8 from = m.from;
  uint8 to = m.to;
  uint8 type = m.type;
  uint8 moved = Piece(from);
  uint8 captured = Piece(to);
  uint8 promoted = m.promoted;
  uint64 key  = pos->hash;
  uint64 pkey = pos->phash;
  uint32 mkey = pos->mhash;
	
  if(type == 0 || type == DC)
  { 
    //stack update:
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = 0;

    //position update:
    pos->square[from] = 0;
    pos->square[to] = moved;
    bitclear(pos->occ[OCC_W], from);
    bitclear(pos->occ[moved], from);
    bitset(pos->occ[OCC_W], to);
    bitset(pos->occ[moved], to);

    //hash update:
    key ^= zobrist_psq[moved][from];
    key ^= zobrist_psq[moved][to];
    
    //pawn hash update:
    if(moved == WP)
    { pkey ^= zobrist_psq[WP][from];
      pkey ^= zobrist_psq[WP][to];
    }
    else if(moved == WK) pos->ksq[W] = to;
  }
  else if(type & PROM)
  {	
    pkey ^= zobrist_psq[WP][from];
    mkey ^= zobrist_material[WP][pos->pcount[WP]--];
    mkey ^= zobrist_material[WP][pos->pcount[WP]];
    mkey ^= zobrist_material[promoted][pos->pcount[promoted]++];
    mkey ^= zobrist_material[promoted][pos->pcount[promoted]];

    if(type & CAP)
    { 
      mkey ^= zobrist_material[captured][pos->pcount[captured]--];
      mkey ^= zobrist_material[captured][pos->pcount[captured]];

      //stack info:
      pos->stack[sply].moved = WP;
      pos->stack[sply].captured = captured;

      //position info:
      pos->square[from] = 0;
      pos->square[to] = promoted;

      //clear the moved:
      bitclear(pos->occ[OCC_W], from);
      bitclear(pos->occ[WP], from);
      
      //clear the captured:
      bitclear(pos->occ[OCC_B], to);
      bitclear(pos->occ[captured], to);

      //update:
      bitset(pos->occ[OCC_W], to);
      bitset(pos->occ[promoted], to);

      //hash update:
      key ^= zobrist_psq[WP][from];
      key ^= zobrist_psq[captured][to];
      key ^= zobrist_psq[promoted][to];
    }
    else 
    { //stack info:
      pos->stack[sply].moved = WP;
      pos->stack[sply].captured = 0;

      //position info:
      pos->square[from] = 0;
      pos->square[to] = promoted;
      bitclear(pos->occ[OCC_W], from);
      bitclear(pos->occ[WP], from);
      bitset(pos->occ[OCC_W], to);
      bitset(pos->occ[promoted], to);

      //hash update:
      key ^= zobrist_psq[WP][from];
      key ^= zobrist_psq[promoted][to];
    }
  }
	
  else if(type & EP)
  {	
    mkey ^= zobrist_material[BP][pos->pcount[BP]--];
    mkey ^= zobrist_material[BP][pos->pcount[BP]];
    
    //stack info:
    pos->stack[sply].moved = WP;
    pos->stack[sply].captured = BP;

    //position info:
    pos->square[from] = 0;
    pos->square[to] = WP;
    pos->square[to-8] = 0;

    //clear the moved and the captured:
    bitclear(pos->occ[OCC_W], from);
    bitclear(pos->occ[WP], from);
    bitclear(pos->occ[OCC_B], to-8);
    bitclear(pos->occ[BP], to-8);

    //update:
    bitset(pos->occ[OCC_W], to);
    bitset(pos->occ[WP], to);

    //hash update:
    key ^= zobrist_psq[BP][to-8];
    key ^= zobrist_psq[WP][from];
    key ^= zobrist_psq[WP][to];

    //phash
    pkey ^= zobrist_psq[BP][to-8];
    pkey ^= zobrist_psq[WP][from];
    pkey ^= zobrist_psq[WP][to];
  }
  
  else if(type == CAP)
  { moved = pos->square[from];
    captured = pos->square[to];

    if(moved == WP)
    { pkey ^= zobrist_psq[WP][from];
      pkey ^= zobrist_psq[WP][to];
    }
    else if(moved == WK) pos->ksq[W] = to;

    if(captured == BP)
      pkey ^= zobrist_psq[BP][to];
    
    mkey ^= zobrist_material[captured][pos->pcount[captured]--];
    mkey ^= zobrist_material[captured][pos->pcount[captured]];

    //stack info:
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = captured;

    //position info:
    pos->square[from] = 0;
    pos->square[to] = moved;

    //clear the moved:
    bitclear(pos->occ[OCC_W], from);
    bitclear(pos->occ[moved], from);

    //clear the captured:
    bitclear(pos->occ[OCC_B], to);
    bitclear(pos->occ[captured], to);

    //update:
    bitset(pos->occ[OCC_W], to);
    bitset(pos->occ[moved], to);

    //hash update:
    key ^= zobrist_psq[moved][from];
    key ^= zobrist_psq[captured][to];
    key ^= zobrist_psq[moved][to];
  }

  else if(type & CASTLE)
  {
    if(type & WHITE_OO)
    {
      //stack:
      pos->stack[sply].moved = WK;
      pos->stack[sply].captured = 0;

      //position:
      pos->square[G1] = WK;
      pos->square[E1] = 0;
      pos->square[F1] = WR;
      pos->square[H1] = 0;

      //king:
      bitclear(pos->occ[OCC_W], E1);
      bitset(pos->occ[OCC_W], G1);
      pos->occ[WK] = 1ULL << G1;

      //rook:
      bitclear(pos->occ[OCC_W], H1);
      bitclear(pos->occ[WR], H1);
      bitset(pos->occ[OCC_W], F1);
      bitset(pos->occ[WR], F1);

      //hash update:
      key ^= zobrist_psq[WK][E1];
      key ^= zobrist_psq[WR][H1];
      key ^= zobrist_psq[WK][G1];
      key ^= zobrist_psq[WR][F1];
      
      //ksq
      pos->ksq[W] = G1;
    }
    else if(type & WHITE_OOO)
    {
      //stack:
      pos->stack[sply].moved = WK;
      pos->stack[sply].captured = 0;

      //position:
      pos->square[C1] = WK;
      pos->square[E1] = 0;
      pos->square[D1] = WR;
      pos->square[A1] = 0;

      //king:
      bitclear(pos->occ[OCC_W], E1);
      bitset(pos->occ[OCC_W], C1);
      pos->occ[WK] = 1ULL << C1;

      //rook:
      bitclear(pos->occ[OCC_W], A1);
      bitclear(pos->occ[WR], A1);
      bitset(pos->occ[OCC_W], D1);
      bitset(pos->occ[WR], D1);

      //hash update:
      key ^= zobrist_psq[WK][E1];
      key ^= zobrist_psq[WR][A1];
      key ^= zobrist_psq[WK][C1];
      key ^= zobrist_psq[WR][D1];
      
      //ksq
      pos->ksq[W] = C1;
    }
  }
	
  //castling flags update:
  pos->stack[sply].castle = pos->castle; //store previous
  if(moved == WK)//update if needed
  { key ^= zobrist_castle[pos->castle];
    pos->castle &= ~(WHITE_OO | WHITE_OOO);
    key ^= zobrist_castle[pos->castle];
  }
  else if(moved == WR)
  { if(from == H1) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~WHITE_OO;
      key ^= zobrist_castle[pos->castle];
    }
    else if(from == A1) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~WHITE_OOO;
      key ^= zobrist_castle[pos->castle];
    }
  }

  //e.p. and mr50 update:
  pos->stack[sply].ep = pos->ep;
  if(moved == WP)
  { pos->mr50 = 0;
    if((to - from) == 16)
    { key ^= zobrist_ep[pos->ep];
      pos->ep = from + 8;
      key ^= zobrist_ep[pos->ep];
    }
    else 
    { key ^= zobrist_ep[pos->ep];
      pos->ep = 0;
    }
  }
  else
  { key ^= zobrist_ep[pos->ep];
    pos->ep = 0;
    if(type & CAP) pos->mr50 = 0;
    else pos->mr50++;
  }

  //move store:
  pos->stack[sply].m = m;

  //btm after this move, turn on btm.
  key ^= zobrist_btm;

  //hash store:
  pos->stack[sply].hash = pos->hash;
  pos->stack[sply].phash = pos->phash;
  pos->stack[sply].mhash = pos->mhash;

  pos->hash = key;
  pos->phash = pkey;
  pos->mhash = mkey;

  //stored!:
  pos->ply++;
  pos->sply++;
  pos->side = B;
}


static void move_unmake_w()
{
  int sply = pos->sply - 1;
  move_t m = pos->stack[sply].m;
  uint8 moved = pos->stack[sply].moved;
  uint8 captured = pos->stack[sply].captured;
  uint8 from = m.from;
  uint8 to   = m.to;
  uint8 type = m.type;
  uint8 promoted = m.promoted;

  if(type == 0 || type == DC)
  {
    pos->square[from] = moved;
    pos->square[to] = 0;
    bitclear(pos->occ[OCC_W], to);
    bitclear(pos->occ[moved], to);
    bitset(pos->occ[OCC_W], from);
    bitset(pos->occ[moved], from);
    if(moved == WK) pos->ksq[W] = from;
  }
  else if(type & PROM)
  { bitclear(pos->occ[OCC_W], to);
    bitclear(pos->occ[promoted], to);
    pos->square[from] = WP;
    bitset(pos->occ[OCC_W], from);
    bitset(pos->occ[WP], from);
    pos->pcount[WP]++;
    pos->pcount[promoted]--;
    
    if(type & CAP)
    { pos->square[to] = captured;
      bitset(pos->occ[captured], to);
      bitset(pos->occ[OCC_B], to);
      pos->pcount[captured]++;
    }
    else pos->square[to] = 0;
  }

  else if(type & EP)
  { pos->square[to] = 0;
    bitclear(pos->occ[OCC_W], to);
    bitclear(pos->occ[WP], to);

    pos->square[from] = WP;
    bitset(pos->occ[OCC_W], from);
    bitset(pos->occ[WP], from);

    pos->square[to - 8] = BP;
    bitset(pos->occ[OCC_B], (to - 8));
    bitset(pos->occ[BP],(to - 8));
    pos->pcount[BP]++;
  }
  else if(type == CAP)
  {
    pos->square[to] = captured;
    bitset(pos->occ[captured], to);
    bitset(pos->occ[OCC_B], to);

    pos->square[from] = moved;
    bitclear(pos->occ[OCC_W], to);
    bitclear(pos->occ[moved], to);
    bitset(pos->occ[OCC_W], from);
    bitset(pos->occ[moved], from);
    pos->pcount[captured]++;
    if(moved == WK) pos->ksq[W] = from;
  }

  else if(type & CASTLE)
  {
    if(type & WHITE_OO)
    { pos->square[G1] = 0;
      pos->square[E1] = WK;
      pos->square[F1] = 0;
      pos->square[H1] = WR;
      bitclear(pos->occ[OCC_W], G1);
      bitclear(pos->occ[OCC_W], F1);
      bitset(pos->occ[OCC_W], E1);
      bitset(pos->occ[OCC_W], H1);
      pos->occ[WK] = 1ULL << E1;
      bitclear(pos->occ[WR], F1);
      bitset(pos->occ[WR], H1);
      pos->ksq[W] = E1;
    }
    else if(type & WHITE_OOO)
    { pos->square[C1] = 0;
      pos->square[E1] = WK;
      pos->square[D1] = 0;
      pos->square[A1] = WR;
      bitclear(pos->occ[OCC_W], C1);
      bitclear(pos->occ[OCC_W], D1);
      bitset(pos->occ[OCC_W], E1);
      bitset(pos->occ[OCC_W], A1);
      pos->occ[WK] = 1ULL << E1;
      bitclear(pos->occ[WR], D1);
      bitset(pos->occ[WR], A1);
      pos->ksq[W] = E1;
    }
  }

  if(moved != WP && !(type & CAP)) pos->mr50--;

  pos->ep = pos->stack[sply].ep;
  pos->castle = pos->stack[sply].castle;
  pos->hash = pos->stack[sply].hash;
  pos->phash = pos->stack[sply].phash;
  pos->mhash = pos->stack[sply].mhash;
    
  pos->sply--; 
  pos->ply--;
  pos->side = W;
}

static void move_make_b(move_t m)
{
  int sply = pos->sply;
  uint8 from = m.from;
  uint8 to = m.to;
  uint8 type = m.type;
  uint8 moved = Piece(from);
  uint8 captured = Piece(to);
  uint8 promoted = m.promoted;
  uint64 key  = pos->hash;
  uint64 pkey = pos->phash;
  uint32 mkey = pos->mhash;

  if(type == 0 || type == DC)
  { 
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = 0;

    //position update:
    pos->square[from] = 0;
    pos->square[to] = moved;
    bitclear(pos->occ[OCC_B], from);
    bitclear(pos->occ[moved], from);
    bitset(pos->occ[OCC_B], to);
    bitset(pos->occ[moved], to);

    //hash update:
    key ^= zobrist_psq[moved][from];
    key ^= zobrist_psq[moved][to];

    //pawn hash update:
    if(moved == BP)
    { pkey ^= zobrist_psq[BP][from];
      pkey ^= zobrist_psq[BP][to];
    }
    else if(moved == BK) pos->ksq[B] = to;
  }
  else if(type & PROM)
  {	
    pkey ^= zobrist_psq[BP][from];    
    mkey ^= zobrist_material[BP][pos->pcount[BP]--];
    mkey ^= zobrist_material[BP][pos->pcount[BP]];
    mkey ^= zobrist_material[promoted][pos->pcount[promoted]++];
    mkey ^= zobrist_material[promoted][pos->pcount[promoted]];

    if(type & CAP) 
    { 
      captured = pos->square[to];
      mkey ^= zobrist_material[captured][pos->pcount[captured]--];
      mkey ^= zobrist_material[captured][pos->pcount[captured]];
      
      //stack info:
      pos->stack[sply].moved = BP;
      pos->stack[sply].captured = captured;

      //position info:
      pos->square[from] = 0;
      pos->square[to] = promoted;

      //clear the moved:
      bitclear(pos->occ[OCC_B], from);
      bitclear(pos->occ[BP], from);

      //clear the captured:
      bitclear(pos->occ[OCC_W], to);
      bitclear(pos->occ[captured], to);

      //update:
      bitset(pos->occ[OCC_B], to);
      bitset(pos->occ[promoted], to);

      //hash update:
      key ^= zobrist_psq[BP][from];
      key ^= zobrist_psq[captured][to];
      key ^= zobrist_psq[promoted][to];
    }
    else
    { //stack info:
      pos->stack[sply].moved = BP;
      pos->stack[sply].captured = 0;
      
      //position info:
      pos->square[from] = 0;
      pos->square[to] = promoted;
      bitclear(pos->occ[OCC_B], from);
      bitclear(pos->occ[BP], from);
      bitset(pos->occ[OCC_B], to);
      bitset(pos->occ[promoted], to);

      //hash update:
      key ^= zobrist_psq[BP][from];
      key ^= zobrist_psq[promoted][to];
    }
  }

  else if(type & EP)
  {
    mkey ^= zobrist_material[WP][pos->pcount[WP]--];
    mkey ^= zobrist_material[WP][pos->pcount[WP]];

    //stack info:
    pos->stack[sply].moved = BP;
    pos->stack[sply].captured = WP;

    //position info:
    pos->square[from] = 0;
    pos->square[to] = BP;
    pos->square[to + 8] = 0;

    //clear the moved and the captured:
    bitclear(pos->occ[OCC_B], from);
    bitclear(pos->occ[BP], from);
    bitclear(pos->occ[OCC_W], to + 8);
    bitclear(pos->occ[WP], to + 8);

    //update:
    bitset(pos->occ[OCC_B], to);
    bitset(pos->occ[BP], to);

    //hash update:
    key ^= zobrist_psq[WP][to + 8];
    key ^= zobrist_psq[BP][from];
    key ^= zobrist_psq[BP][to];

    //phash
    pkey ^= zobrist_psq[WP][to + 8];
    pkey ^= zobrist_psq[BP][from];
    pkey ^= zobrist_psq[BP][to];
  }
  else if(type == CAP)
  { 
    if(moved == BP)
    { pkey ^= zobrist_psq[BP][from];
      pkey ^= zobrist_psq[BP][to];
    }
    else if(moved == BK) pos->ksq[B] = to;

    if(captured == WP)
      pkey ^= zobrist_psq[WP][to];
    
    mkey ^= zobrist_material[captured][pos->pcount[captured]--];
    mkey ^= zobrist_material[captured][pos->pcount[captured]];
    
    //stack info:
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = captured;

    //position info:
    pos->square[from] = 0;
    pos->square[to] = moved;

    //clear the moved:
    bitclear(pos->occ[OCC_B], from);
    bitclear(pos->occ[moved], from);

    //clear the captured:
    bitclear(pos->occ[OCC_W], to);
    bitclear(pos->occ[captured], to);

    //update:
    bitset(pos->occ[OCC_B], to);
    bitset(pos->occ[moved], to);
    
    //hash update:
    key ^= zobrist_psq[moved][from];
    key ^= zobrist_psq[captured][to];
    key ^= zobrist_psq[moved][to];
  }

  else if(type & CASTLE)
  {
    if(type & BLACK_OO)
    {
      //position:
      pos->square[G8] = BK;
      pos->square[E8] = 0;
      pos->square[F8] = BR;
      pos->square[H8] = 0;

      //king:
      bitclear(pos->occ[OCC_B], E8);
      bitset(pos->occ[OCC_B], G8);
      pos->occ[BK] = 1ULL << G8;

      //rook:
      bitclear(pos->occ[OCC_B], H8);
      bitclear(pos->occ[BR], H8);
      bitset(pos->occ[OCC_B], F8);
      bitset(pos->occ[BR], F8);

      //hash update:
      key ^= zobrist_psq[BK][E8];
      key ^= zobrist_psq[BR][H8];
      key ^= zobrist_psq[BK][G8];
      key ^= zobrist_psq[BR][F8];
      
      //ksq
      pos->ksq[B] = G8;
    }
    else if(type & BLACK_OOO)
    {
      //stack:
      pos->stack[sply].moved = BK;
      pos->stack[sply].captured = 0;

      //position:
      pos->square[C8] = BK;
      pos->square[E8] = 0;
      pos->square[D8] = BR;
      pos->square[A8] = 0;

      //king:
      bitclear(pos->occ[OCC_B], E8);
      bitset(pos->occ[OCC_B], C8);
      pos->occ[BK] = 1ULL << C8;

      //rook:
      bitclear(pos->occ[OCC_B], A8);
      bitclear(pos->occ[BR], A8);
      bitset(pos->occ[OCC_B], D8);
      bitset(pos->occ[BR], D8);

      //hash update:
      key ^= zobrist_psq[BK][E8];
      key ^= zobrist_psq[BR][A8];
      key ^= zobrist_psq[BK][C8];
      key ^= zobrist_psq[BR][D8];
      
      //ksq
      pos->ksq[B] = C8;
    }
  }

  //castling flags update:
  pos->stack[sply].castle = pos->castle; //store previous
  if(moved == BK)//update if needed
  { key ^= zobrist_castle[pos->castle];
    pos->castle &= ~(BLACK_OO | BLACK_OOO);
    key ^= zobrist_castle[pos->castle];
  }
  else if(moved == BR)
  { if(from == H8) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~BLACK_OO;
      key ^= zobrist_castle[pos->castle];
    }
    else if(from == A8)
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~BLACK_OOO;
      key ^= zobrist_castle[pos->castle];
    }
  }

  //e.p. and mr50 update:
  pos->stack[sply].ep = pos->ep;
  if(moved == BP)
  { pos->mr50 = 0;
    if((from - to) == 16)
    { key ^= zobrist_ep[pos->ep];
      pos->ep = to + 8;
      key ^= zobrist_ep[pos->ep];
    }
    else 
    { key ^= zobrist_ep[pos->ep];
      pos->ep = 0;
    }
  }
  else
  { key ^= zobrist_ep[pos->ep];
    pos->ep = 0;
    if(type & CAP) pos->mr50 = 0;
    else pos->mr50++;
  }

  //move store:
  pos->stack[sply].m = m;

  //wtm after this move, turn off btm.
  key ^= zobrist_btm;

  //hash store:
  pos->stack[sply].hash = pos->hash;
  pos->stack[sply].phash = pos->phash;
  pos->stack[sply].mhash = pos->mhash;

  pos->hash = key;
  pos->phash = pkey;
  pos->mhash = mkey;

  //stored!:
  pos->ply++;
  pos->sply++;
  pos->side = W;
  pos->fullmove++;
}

static void move_unmake_b()
{
  int sply = pos->sply - 1;
  move_t m = pos->stack[sply].m;
  uint8 moved = pos->stack[sply].moved;
  uint8 captured = pos->stack[sply].captured;
  uint8 from = m.from;
  uint8 to   = m.to;
  uint8 type = m.type;
  uint8 promoted = m.promoted;

  if(type == 0 || type == DC)
  { pos->square[from] = moved;
    pos->square[to] = 0;
    bitclear(pos->occ[OCC_B], to);
    bitclear(pos->occ[moved], to);
    bitset(pos->occ[OCC_B], from);
    bitset(pos->occ[moved], from);
    if(moved == BK) pos->ksq[B] = from;
  }
  else if(type & PROM)
  { bitclear(pos->occ[OCC_B], to);
    bitclear(pos->occ[promoted], to);
    pos->square[from] = BP;
    bitset(pos->occ[OCC_B], from);
    bitset(pos->occ[BP], from);
    pos->pcount[BP]++;
    pos->pcount[promoted]--;
    
    if(type & CAP)
    { pos->square[to] = captured;
      bitset(pos->occ[captured], to);
      bitset(pos->occ[OCC_W], to);
      pos->pcount[captured]++;
    }
    else pos->square[to] = 0;
  }
  else if(type & EP)
  { pos->square[to] = 0;
    bitclear(pos->occ[OCC_B], to);
    bitclear(pos->occ[BP], to);

    pos->square[from] = BP;
    bitset(pos->occ[OCC_B], from);
    bitset(pos->occ[BP], from);

    pos->square[to + 8] = WP;
    bitset(pos->occ[OCC_W], (to + 8));
    bitset(pos->occ[WP], (to + 8));
    pos->pcount[WP]++;
  }
  else if(type == CAP)
  {
    pos->square[to] = captured;
    bitset(pos->occ[captured], to);
    bitset(pos->occ[OCC_W], to);

    pos->square[from] = moved;
    bitclear(pos->occ[OCC_B], to);
    bitclear(pos->occ[moved], to);
    bitset(pos->occ[OCC_B], from);
    bitset(pos->occ[moved], from);
    pos->pcount[captured]++;
    if(moved == BK) pos->ksq[B] = from;
  }
  else if(type & CASTLE)
  {
    if(type & BLACK_OO)
    { pos->square[G8] = 0;
      pos->square[E8] = BK;
      pos->square[F8] = 0;
      pos->square[H8] = BR;
      bitclear(pos->occ[OCC_B], G8);
      bitclear(pos->occ[OCC_B], F8);
      bitset(pos->occ[OCC_B], E8);
      bitset(pos->occ[OCC_B], H8);
      pos->occ[BK] = 1ULL << E8;
      bitclear(pos->occ[BR], F8);
      bitset(pos->occ[BR], H8);
      pos->ksq[B] = E8;
    }
    else if(type & BLACK_OOO)
    { pos->square[C8] = 0;
      pos->square[E8] = BK;
      pos->square[D8] = 0;
      pos->square[A8] = BR;
      bitclear(pos->occ[OCC_B], C8);
      bitclear(pos->occ[OCC_B], D8);
      bitset(pos->occ[OCC_B],E8);
      bitset(pos->occ[OCC_B],A8);
      pos->occ[BK] = 1ULL << E8;
      bitclear(pos->occ[BR], D8);
      bitset(pos->occ[BR], A8);
      pos->ksq[B] = E8;
    }
  }
  if(moved != BP && !(type & CAP)) pos->mr50--;

  pos->ep = pos->stack[sply].ep;
  pos->castle = pos->stack[sply].castle;
  pos->hash = pos->stack[sply].hash;
  pos->phash = pos->stack[sply].phash;
  pos->mhash = pos->stack[sply].mhash;
  
  pos->sply--; 
  pos->ply--;
  pos->side = B;
  pos->fullmove--;
}

void move_make(move_t m)
{ if(pos->side) move_make_b(m);
  else move_make_w(m);
}

void move_unmake()
{ if(pos->side) move_unmake_w();
  else move_unmake_b();
}

bool make_if_legal(move_t m)
{	
  int i;
  int move_count; 
  bool found = 0; 
  int index = 0; 
  move_t ms[MSSIZE];

  move_count = move_gen(&ms[0]);
  for(i = 0; i < move_count; i++)
  { //.type is omitted since this is a
    //direct input (whithout type).
    if(m.from == ms[i].from
    && m.to == ms[i].to
    && m.promoted == ms[i].promoted)
    { found = true;
      index = i;
      break;
    }
  }
  if(found)
  { move_make(ms[index]);
    return true;
  }
  return false;
}

void move_make_null(nullstack_t *ns)
{ ns->side   = pos->side;
  ns->fifty  = pos->mr50;
  ns->flags  = pos->castle;
  ns->old_ep = pos->ep;
  ns->hash   = pos->hash;
  pos->ep    = 0;
  pos->mr50  = 0;
  pos->side ^= 1;
  pos->hash ^= zobrist_btm;
  pos->hash ^= zobrist_ep[ns->old_ep];
  pos->ply++;
}

void move_unmake_null(nullstack_t *ns)
{ pos->side   = ns->side;
  pos->mr50   = ns->fifty;
  pos->castle = ns->flags;
  pos->ep     = ns->old_ep;
  pos->hash   = ns->hash;
  pos->ply--;
}

int move_illegal(move_t m)
{//quick leagality test:
  if(!(m.type & EP))
  { if(GetColor(pos->square[m.from]) == W)
    { if(pos->square[m.from] != WK
      && is_pinned_w(m.from, m.to)) return 1;
    }
    else
    { if(pos->square[m.from] != BK
      && is_pinned_b(m.from, m.to)) return 1;
    }
  }
  return 0;
}

bool is_legal(move_t m)
{ int i, move_count; 
  move_t ms[MSSIZE];
  move_count = move_gen(ms);
  for(i = 0; i < move_count; i++)
  { if(m.p == ms[i].p)
    { if(!move_illegal(ms[i]))
        return true;
      break;  
    }
  }
  return false;
}

bool validate_move(uint32 move)
{
  #ifndef VALIDATE_BESTMOVE
  if(move == 0) return false;
  return true;
  #else
  move_t m;
  bitboard_t occ;
  int stm;
  
  if(move == 0) return false;
  m.p = move;
  stm = pos->side;
  
  //general considerations:
  if(m.from == m.to) return false;
  if(Piece(m.from) == 0) return false;
  if(GetColor(Piece(m.from)) != stm
  ||(Piece(m.to) && GetColor(Piece(m.to)) == stm))
    return false;
  if(m.type & CAP)
  { if((!(m.type & EP) && Piece(m.to) == EMPTY) || Piece(m.to) == OpColoured(KING, stm))
      return false;
  }
  if(m.type & PROM)
  { if(Piece(m.from) != Coloured(PAWN, stm)) return false;
    if(stm == W && Rank(m.from) != RANK_7) return false;
    if(stm == B && Rank(m.from) != RANK_2) return false;
  }
  
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  if(stm == W)
  { ///King moves:
    if(Piece(m.from) == WK && is_sq_attacked_b(m.to)) return false;
    //castling:
    if(m.type & CASTLE)
    { if(m.from != E1 || Piece(E1) != WK) return false;
      if(m.type & WHITE_OO)
      { if(m.to != G1) return false;
        if(Piece(H1) != WR) return false;
        if(occ & W_OO_MASK) return false;
        if(is_sq_attacked_b(E1) || is_sq_attacked_b(F1) || is_sq_attacked_b(G1))
          return false;
      }
      if(m.type & WHITE_OOO)
      { if(m.to != C1) return false;
        if(Piece(A1) != WR) return false;
        if(occ & W_OOO_MASK) return false;
        if(is_sq_attacked_b(E1) || is_sq_attacked_b(D1) || is_sq_attacked_b(C1))
          return false;
      }
    }
    ///Pawn moves:
    if(m.type & EP)
    { if(Piece(m.from) != WP || pos->ep == 0) return false;
      if(Piece(pos->ep - 8) != BP) return false;
      if(pos->ep - 7 != m.from && pos->ep - 9 != m.from) return false;
      if(pos->ep - 7 == m.from)
      { if(!(p_attacks[W][pos->ep-7] & (1ULL << pos->ep)))
          return false;
        if(is_pinned_ep_w((pos->ep-7), pos->ep))
          return false;
      }
      if(pos->ep - 9 == m.from)
      { if(!(p_attacks[W][pos->ep-9] & (1ULL << pos->ep)))
          return false;
        if(is_pinned_ep_w((pos->ep-9), pos->ep)) 
          return false;
      }
    }
    else
    { //pawn moves except EP:
      if(Piece(m.from) == WP)
      { if(Rank(m.from) == RANK_7 && (!m.promoted || !(m.type & PROM))) return false;
        if(m.type & CAP)
        { if(m.from == m.to - 9)
            if(!(p_attacks[W][m.from] & (1ULL << m.to)))
              return false;
          else if(m.from == m.to - 7)
            if(!(p_attacks[W][m.from] & (1ULL << m.to)))
              return false;
          else return false;
        }
        else
        { if(m.to - m.from == 8)
            if(pos->square[m.to]) return false;
          else if(m.to - m.from == 16)
            if(pos->square[m.from + 8] || pos->square[m.from + 16]) return false;
          else return false;
        }
      }
    }
  }
  else ///BLACK on move:
  { ///King moves:
    if(Piece(m.from) == BK && is_sq_attacked_w(m.to)) return false;
    //castling:
    if(m.type & CASTLE)
    { if(m.from != E8 || Piece(E8) != BK) return false;
      if(m.type & BLACK_OO)
      { if(m.to != G8) return false;
        if(Piece(H8) != BR) return false;
        if(occ & B_OO_MASK) return false;
        if(is_sq_attacked_w(E8) || is_sq_attacked_w(F8) || is_sq_attacked_w(G8))
          return false;
      }
      if(m.type & BLACK_OOO)
      { if(m.to != C8) return false;
        if(Piece(A8) != BR) return false;
        if(occ & B_OOO_MASK) return false;
        if(is_sq_attacked_w(E8) || is_sq_attacked_w(D8) || is_sq_attacked_w(C8))
          return false;
      }
      ///Pawn moves:
      if(m.type & EP)
      { if(Piece(m.from) != BP || pos->ep == 0) return false;
        if(Piece(pos->ep + 8) != WP) return false;
        if(pos->ep + 7 != m.from && pos->ep + 9 != m.from) return false;
        if(pos->ep + 7 == m.from)
        { if(!(p_attacks[B][pos->ep+7] & (1ULL << pos->ep)))
            return false;
          if(is_pinned_ep_b((pos->ep+7), pos->ep))
            return false;
        }
        if(pos->ep + 9 == m.from)
        { if(!(p_attacks[B][pos->ep+9] & (1ULL << pos->ep)))
            return false;
          if(is_pinned_ep_b((pos->ep+9), pos->ep))
            return false;
        }
      }
      else
      { //pawn moves except EP:
        if(Piece(m.from) == BP)
        { if(Rank(m.from) == RANK_2 && (!m.promoted || !(m.type & PROM))) return false;
          if(m.type & CAP)
          { if(m.from == m.to + 9)
              if(!(p_attacks[B][m.from] & (1ULL << m.to)))
                return false;
            else if(m.from == m.to + 7)
              if(!(p_attacks[B][m.from] & (1ULL << m.to)))
                return false;
            else return false;
          }
          else
          { if(m.from - m.to == 8)
              if(pos->square[m.to]) return false;
            else if(m.from - m.to == 16)
              if(pos->square[m.from - 8] || pos->square[m.from - 16]) return false;
            else return false;
          }
        }
      }
    }
  }
  //Testing the direction vector for obstacles.
  //Also, testing if the move is possible at all,
  //or just two random from/to numbers.
  //That's not expected to happen (nor Type II collision for that matter),
  //but it's included for the sake of completeness.
  switch(GetType(Piece(m.from)))
  {
    case BISHOP:
      if((direction[m.from][m.to] & occ) 
      || (distance[m.from][m.to] > 1 && direction[m.from][m.to] == 0ULL)
      || (Rank(m.from) == Rank(m.to) || File(m.from) == File(m.to)))
        return false;
    break;
      
    case QUEEN:
      if(direction[m.from][m.to] & occ) return false;
      if(distance[m.from][m.to] > 1 && direction[m.from][m.to] == 0ULL) return false;
    break;
    
    case ROOK:
      if(Rank(m.from) != Rank(m.to) && File(m.from) != File(m.to)) return false;
      if(direction[m.from][m.to] & occ) return false;
    break;
    
    case KNIGHT:
      if(!(n_moves[m.from] & (1ULL << m.to))) return 0;
    break;
    
    case KING:
      if(!(m.type & CASTLE) && !(k_moves[m.from] & (1ULL << m.to))) return 0;
    break;
    
    case PAWN: break;
    default:  return false; //Piece type Illegal.
    break;
  }
  //Finally, testing for pseudo/legality:
  return !move_illegal(m);
  #endif
}


