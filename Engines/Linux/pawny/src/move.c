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

static void move_make_w(move_t m)
{
  uint8 moved, captured;
  int sply = pos->sply;
  uint64 key = pos->hash;
  uint64 pkey = pos->phash;
  uint32 mkey = pos->mhash;
	
  if(m.type == 0)
  { //stack update:
    moved = pos->square[m.from];
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = 0;

    //position update:
    pos->square[m.from] = 0;
    pos->square[m.to] = moved;
    bitclear(pos->occ[OCC_W], m.from);
    bitclear(pos->occ[moved], m.from);
    bitset(pos->occ[OCC_W], m.to);
    bitset(pos->occ[moved], m.to);

    //hash update:
    key ^= zobrist_psq[moved][m.from];
    key ^= zobrist_psq[moved][m.to];
    
    //pawn hash update:
    if(moved == WP)
    { pkey ^= zobrist_psq[WP][m.from];
      pkey ^= zobrist_psq[WP][m.to];
    }
    else if(moved == WK) pos->ksq[W] = m.to;
  }
  else if(m.type & PROM)
  {	
    pkey ^= zobrist_psq[WP][m.from];
    mkey ^= zobrist_material[WP][pos->pcount[WP]--];
    mkey ^= zobrist_material[WP][pos->pcount[WP]];
    mkey ^= zobrist_material[m.promoted][pos->pcount[m.promoted]++];
    mkey ^= zobrist_material[m.promoted][pos->pcount[m.promoted]];

    if(m.type & CAP)
    { 
      captured = pos->square[m.to];
      mkey ^= zobrist_material[captured][pos->pcount[captured]--];
      mkey ^= zobrist_material[captured][pos->pcount[captured]];

      //stack info:
      pos->stack[sply].moved = WP;
      pos->stack[sply].captured = captured;

      //position info:
      pos->square[m.from] = 0;
      pos->square[m.to] = m.promoted;

      //clear the moved:
      bitclear(pos->occ[OCC_W], m.from);
      bitclear(pos->occ[WP], m.from);
      
      //clear the captured:
      bitclear(pos->occ[OCC_B], m.to);
      bitclear(pos->occ[captured], m.to);

      //update:
      bitset(pos->occ[OCC_W], m.to);
      bitset(pos->occ[m.promoted], m.to);

      //hash update:
      key ^= zobrist_psq[WP][m.from];
      key ^= zobrist_psq[captured][m.to];
      key ^= zobrist_psq[m.promoted][m.to];
    }
    else 
    { //stack info:
      pos->stack[sply].moved = WP;
      pos->stack[sply].captured = 0;

      //position info:
      pos->square[m.from] = 0;
      pos->square[m.to] = m.promoted;
      bitclear(pos->occ[OCC_W], m.from);
      bitclear(pos->occ[WP], m.from);
      bitset(pos->occ[OCC_W], m.to);
      bitset(pos->occ[m.promoted], m.to);

      //hash update:
      key ^= zobrist_psq[WP][m.from];
      key ^= zobrist_psq[m.promoted][m.to];
    }
  }
	
  else if(m.type & EP)
  {	
    mkey ^= zobrist_material[BP][pos->pcount[BP]--];
    mkey ^= zobrist_material[BP][pos->pcount[BP]];
    
    //stack info:
    pos->stack[sply].moved = WP;
    pos->stack[sply].captured = BP;

    //position info:
    pos->square[m.from] = 0;
    pos->square[m.to] = WP;
    pos->square[m.to-8] = 0;

    //clear the moved and the captured:
    bitclear(pos->occ[OCC_W], m.from);
    bitclear(pos->occ[WP], m.from);
    bitclear(pos->occ[OCC_B], m.to-8);
    bitclear(pos->occ[BP], m.to-8);

    //update:
    bitset(pos->occ[OCC_W], m.to);
    bitset(pos->occ[WP], m.to);

    //hash update:
    key ^= zobrist_psq[BP][m.to-8];
    key ^= zobrist_psq[WP][m.from];
    key ^= zobrist_psq[WP][m.to];

    //phash
    pkey ^= zobrist_psq[BP][m.to-8];
    pkey ^= zobrist_psq[WP][m.from];
    pkey ^= zobrist_psq[WP][m.to];
	}
  
  else if(m.type == CAP)
  { moved = pos->square[m.from];
    captured = pos->square[m.to];

    if(moved == WP)
    { pkey ^= zobrist_psq[WP][m.from];
      pkey ^= zobrist_psq[WP][m.to];
    }
    else if(moved == WK) pos->ksq[W] = m.to;

    if(captured == BP)
      pkey ^= zobrist_psq[BP][m.to];
    
    mkey ^= zobrist_material[captured][pos->pcount[captured]--];
    mkey ^= zobrist_material[captured][pos->pcount[captured]];

    //stack info:
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = captured;

    //position info:
    pos->square[m.from] = 0;
    pos->square[m.to] = moved;

    //clear the moved:
    bitclear(pos->occ[OCC_W], m.from);
    bitclear(pos->occ[moved], m.from);

    //clear the captured:
    bitclear(pos->occ[OCC_B], m.to);
    bitclear(pos->occ[captured], m.to);

    //update:
    bitset(pos->occ[OCC_W], m.to);
    bitset(pos->occ[moved], m.to);

    //hash update:
    key ^= zobrist_psq[moved][m.from];
    key ^= zobrist_psq[captured][m.to];
    key ^= zobrist_psq[moved][m.to];
  }

  else if(m.type & CASTLE)
  {
    if(m.type & WHITE_OO)
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
    else if(m.type & WHITE_OOO)
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
  if(pos->stack[sply].moved == WK)//update if needed
  { key ^= zobrist_castle[pos->castle];
    pos->castle &= ~(WHITE_OO | WHITE_OOO);
    key ^= zobrist_castle[pos->castle];
  }
  else if(pos->stack[sply].moved == WR)
  { if(m.from == H1) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~WHITE_OO;
      key ^= zobrist_castle[pos->castle];
    }
    else if(m.from == A1) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~WHITE_OOO;
      key ^= zobrist_castle[pos->castle];
    }
  }

  //e.p. and mr50 update:
  pos->stack[sply].ep = pos->ep;
  if(pos->stack[sply].moved == WP)
  { pos->mr50 = 0;
    if((m.to - m.from) == 16)
    { key ^= zobrist_ep[pos->ep];
      pos->ep = m.from + 8;
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
    if(m.type & CAP) pos->mr50 = 0;
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
  register int sply;
  uint8 moved;
  move_t m;

  if(pos->sply <= 0) return;
  pos->sply--; 
  pos->ply--;
  pos->side = W;

  sply = pos->sply;
  m = pos->stack[sply].m;

  if(m.type == 0)
  { moved = pos->stack[sply].moved;
    pos->square[m.from] = moved;
    pos->square[m.to] = 0;
    bitclear(pos->occ[OCC_W], m.to);
    bitclear(pos->occ[moved], m.to);
    bitset(pos->occ[OCC_W], m.from);
    bitset(pos->occ[moved], m.from);
    if(moved == WK) pos->ksq[W] = m.from;
  }
  else if(m.type & PROM)
  { bitclear(pos->occ[OCC_W], m.to);
    bitclear(pos->occ[m.promoted], m.to);
    pos->square[m.from] = WP;
    bitset(pos->occ[OCC_W], m.from);
    bitset(pos->occ[WP], m.from);
    pos->pcount[WP]++;
    pos->pcount[m.promoted]--;
    
    if(m.type & CAP)
    { pos->square[m.to] = pos->stack[sply].captured;
      bitset(pos->occ[pos->stack[sply].captured], m.to);
      bitset(pos->occ[OCC_B], m.to);
      pos->pcount[pos->stack[sply].captured]++;
		}
    else pos->square[m.to] = 0;
  }

  else if(m.type & EP)
  { pos->square[m.to] = 0;
    bitclear(pos->occ[OCC_W], m.to);
    bitclear(pos->occ[WP], m.to);

    pos->square[m.from] = WP;
    bitset(pos->occ[OCC_W], m.from);
    bitset(pos->occ[WP], m.from);

    pos->square[m.to - 8] = BP;
    bitset(pos->occ[OCC_B], (m.to - 8));
    bitset(pos->occ[BP],(m.to - 8));
    pos->pcount[BP]++;
  }
  else if(m.type == CAP)
  {
    pos->square[m.to] = pos->stack[sply].captured;
    bitset(pos->occ[pos->stack[sply].captured], m.to);
    bitset(pos->occ[OCC_B], m.to);

    moved = pos->stack[sply].moved;
    pos->square[m.from] = moved;
    bitclear(pos->occ[OCC_W], m.to);
    bitclear(pos->occ[moved], m.to);
    bitset(pos->occ[OCC_W], m.from);
    bitset(pos->occ[moved], m.from);
    pos->pcount[pos->stack[sply].captured]++;
    if(moved == WK) pos->ksq[W] = m.from;
  }

  else if(m.type & CASTLE)
  {
    if(m.type & WHITE_OO)
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
    else if(m.type & WHITE_OOO)
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

  if(pos->stack[sply].moved != WP
  && !(m.type & CAP)) pos->mr50--;

  pos->ep = pos->stack[sply].ep;
  pos->castle = pos->stack[sply].castle;
  pos->hash = pos->stack[sply].hash;
  pos->phash = pos->stack[sply].phash;
  pos->mhash = pos->stack[sply].mhash;
}

static void move_make_b(move_t m)
{
  uint8 moved, captured;
  int sply = pos->sply;
  uint64 key = pos->hash;
  uint64 pkey = pos->phash;
  uint32 mkey = pos->mhash;

  if(m.type == 0)
  { //stack update:
    moved = pos->square[m.from];
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = 0;

    //position update:
    pos->square[m.from] = 0;
    pos->square[m.to] = moved;
    bitclear(pos->occ[OCC_B], m.from);
    bitclear(pos->occ[moved], m.from);
    bitset(pos->occ[OCC_B], m.to);
    bitset(pos->occ[moved], m.to);

    //hash update:
    key ^= zobrist_psq[moved][m.from];
    key ^= zobrist_psq[moved][m.to];

    //pawn hash update:
    if(moved == BP)
    { pkey ^= zobrist_psq[BP][m.from];
      pkey ^= zobrist_psq[BP][m.to];
    }
    else if(moved == BK) pos->ksq[B] = m.to;
  }
  else if(m.type & PROM)
  {	
    pkey ^= zobrist_psq[BP][m.from];    
    mkey ^= zobrist_material[BP][pos->pcount[BP]--];
    mkey ^= zobrist_material[BP][pos->pcount[BP]];
    mkey ^= zobrist_material[m.promoted][pos->pcount[m.promoted]++];
    mkey ^= zobrist_material[m.promoted][pos->pcount[m.promoted]];

    if(m.type & CAP) 
    { 
      captured = pos->square[m.to];
      mkey ^= zobrist_material[captured][pos->pcount[captured]--];
      mkey ^= zobrist_material[captured][pos->pcount[captured]];
      
      //stack info:
      pos->stack[sply].moved = BP;
      pos->stack[sply].captured = captured;

      //position info:
      pos->square[m.from] = 0;
      pos->square[m.to] = m.promoted;

      //clear the moved:
      bitclear(pos->occ[OCC_B], m.from);
      bitclear(pos->occ[BP], m.from);

      //clear the captured:
      bitclear(pos->occ[OCC_W], m.to);
      bitclear(pos->occ[captured], m.to);

      //update:
      bitset(pos->occ[OCC_B], m.to);
      bitset(pos->occ[m.promoted], m.to);

      //hash update:
      key ^= zobrist_psq[BP][m.from];
      key ^= zobrist_psq[captured][m.to];
      key ^= zobrist_psq[m.promoted][m.to];
		}
    else
    { //stack info:
      pos->stack[sply].moved = BP;
      pos->stack[sply].captured = 0;
      
      //position info:
      pos->square[m.from] = 0;
      pos->square[m.to] = m.promoted;
      bitclear(pos->occ[OCC_B], m.from);
      bitclear(pos->occ[BP], m.from);
      bitset(pos->occ[OCC_B], m.to);
      bitset(pos->occ[m.promoted], m.to);

      //hash update:
      key ^= zobrist_psq[BP][m.from];
      key ^= zobrist_psq[m.promoted][m.to];
    }
  }

  else if(m.type & EP)
  {
    mkey ^= zobrist_material[WP][pos->pcount[WP]--];
    mkey ^= zobrist_material[WP][pos->pcount[WP]];

    //stack info:
    pos->stack[sply].moved = BP;
    pos->stack[sply].captured = WP;

    //position info:
    pos->square[m.from] = 0;
    pos->square[m.to] = BP;
    pos->square[m.to+8] = 0;

    //clear the moved and the captured:
    bitclear(pos->occ[OCC_B], m.from);
    bitclear(pos->occ[BP], m.from);
    bitclear(pos->occ[OCC_W], m.to+8);
    bitclear(pos->occ[WP], m.to+8);

    //update:
    bitset(pos->occ[OCC_B], m.to);
    bitset(pos->occ[BP], m.to);

    //hash update:
    key ^= zobrist_psq[WP][m.to+8];
    key ^= zobrist_psq[BP][m.from];
    key ^= zobrist_psq[BP][m.to];

    //phash
    pkey ^= zobrist_psq[WP][m.to+8];
    pkey ^= zobrist_psq[BP][m.from];
    pkey ^= zobrist_psq[BP][m.to];
  }
  else if(m.type == CAP)
  { moved = pos->square[m.from];
    captured = pos->square[m.to];

    if(moved == BP)
    { pkey ^= zobrist_psq[BP][m.from];
      pkey ^= zobrist_psq[BP][m.to];
    }
    else if(moved == BK) pos->ksq[B] = m.to;

    if(captured == WP)
      pkey ^= zobrist_psq[WP][m.to];
    
    mkey ^= zobrist_material[captured][pos->pcount[captured]--];
    mkey ^= zobrist_material[captured][pos->pcount[captured]];
    
    //stack info:
    pos->stack[sply].moved = moved;
    pos->stack[sply].captured = captured;

    //position info:
    pos->square[m.from] = 0;
    pos->square[m.to] = moved;

    //clear the moved:
    bitclear(pos->occ[OCC_B], m.from);
    bitclear(pos->occ[moved], m.from);

    //clear the captured:
    bitclear(pos->occ[OCC_W], m.to);
    bitclear(pos->occ[captured], m.to);

    //update:
    bitset(pos->occ[OCC_B], m.to);
    bitset(pos->occ[moved], m.to);
    
    //hash update:
    key ^= zobrist_psq[moved][m.from];
    key ^= zobrist_psq[captured][m.to];
    key ^= zobrist_psq[moved][m.to];
  }

  else if(m.type & CASTLE)
  {
    if(m.type & BLACK_OO)
    {
      //stack:
      pos->stack[sply].moved = BK;
      pos->stack[sply].captured = 0;

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
    else if(m.type & BLACK_OOO)
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
  if(pos->stack[sply].moved == BK)//update if needed
  { key ^= zobrist_castle[pos->castle];
    pos->castle &= ~(BLACK_OO | BLACK_OOO);
    key ^= zobrist_castle[pos->castle];
  }
  else if(pos->stack[sply].moved == BR)
  { if(m.from == H8) 
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~BLACK_OO;
      key ^= zobrist_castle[pos->castle];
    }
    else if(m.from == A8)
    { key ^= zobrist_castle[pos->castle];
      pos->castle &= ~BLACK_OOO;
      key ^= zobrist_castle[pos->castle];
    }
  }

  //e.p. and mr50 update:
  pos->stack[sply].ep = pos->ep;
  if(pos->stack[sply].moved == BP)
  { pos->mr50 = 0;
    if((m.from - m.to) == 16)
    { key ^= zobrist_ep[pos->ep];
      pos->ep = m.to + 8;
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
    if(m.type & CAP) pos->mr50 = 0;
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
  register int sply;
  uint8 moved;
  move_t m;

  if(pos->sply <= 0) return;
  pos->sply--; 
  pos->ply--;
  pos->side = B;
  pos->fullmove--;

  sply = pos->sply;
  m = pos->stack[sply].m;

  if(m.type == 0)
  { moved = pos->stack[sply].moved;
    pos->square[m.from] = moved;
    pos->square[m.to] = 0;
    bitclear(pos->occ[OCC_B], m.to);
    bitclear(pos->occ[moved], m.to);
    bitset(pos->occ[OCC_B], m.from);
    bitset(pos->occ[moved], m.from);
    if(moved == BK) pos->ksq[B] = m.from;
  }
  else if(m.type & PROM)
  { bitclear(pos->occ[OCC_B], m.to);
    bitclear(pos->occ[m.promoted], m.to);
    pos->square[m.from] = BP;
    bitset(pos->occ[OCC_B], m.from);
    bitset(pos->occ[BP], m.from);
    pos->pcount[BP]++;
    pos->pcount[m.promoted]--;
    
    if(m.type & CAP)
    { pos->square[m.to] = pos->stack[sply].captured;
      bitset(pos->occ[pos->stack[sply].captured], m.to);
      bitset(pos->occ[OCC_W], m.to);
      pos->pcount[pos->stack[sply].captured]++;
    }
    else pos->square[m.to] = 0;
  }
  else if(m.type & EP)
  { pos->square[m.to] = 0;
    bitclear(pos->occ[OCC_B], m.to);
    bitclear(pos->occ[BP], m.to);

    pos->square[m.from] = BP;
    bitset(pos->occ[OCC_B], m.from);
    bitset(pos->occ[BP], m.from);

    pos->square[m.to + 8] = WP;
    bitset(pos->occ[OCC_W], (m.to + 8));
    bitset(pos->occ[WP],(m.to + 8));
    pos->pcount[WP]++;
	}
  else if(m.type == CAP)
  {
    pos->square[m.to] = pos->stack[sply].captured;
    bitset(pos->occ[pos->stack[sply].captured], m.to);
    bitset(pos->occ[OCC_W], m.to);

    moved = pos->stack[sply].moved;
    pos->square[m.from] = moved;
    bitclear(pos->occ[OCC_B], m.to);
    bitclear(pos->occ[moved], m.to);
    bitset(pos->occ[OCC_B], m.from);
    bitset(pos->occ[moved], m.from);
    pos->pcount[pos->stack[sply].captured]++;
    if(moved == BK) pos->ksq[B] = m.from;
	}
  else if(m.type & CASTLE)
  {
    if(m.type & BLACK_OO)
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
    else if(m.type & BLACK_OOO)
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
  if(pos->stack[sply].moved != BP
  && !(m.type & CAP)) pos->mr50--;

  pos->ep = pos->stack[sply].ep;
  pos->castle = pos->stack[sply].castle;
  pos->hash = pos->stack[sply].hash;
  pos->phash = pos->stack[sply].phash;
  pos->mhash = pos->stack[sply].mhash;
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

bool is_legal(move_t m)
{	int i, move_count; 
  move_t ms[MSSIZE];
  move_count = move_gen(&ms[0]);
  for(i = 0; i < move_count; i++)
    if(m.p == ms[i].p) return true;
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
