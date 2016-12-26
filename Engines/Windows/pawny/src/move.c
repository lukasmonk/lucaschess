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

bool move_undo()
{
  int color,hply;
  sq_t from,to;
  uint8 type,p;
  state_t *state = &board->state;
  
  if(!board->hply) return false;
  board->hply--; board->ply--;
  board->side ^= 1; board->xside ^= 1;
  hply = board->hply;
  from = hs[hply].m.to;
  to = hs[hply].m.from;
  color = board->side;
  p = hs[hply].moved;
  type = hs[hply].m.type;
  
  if(type & CASTLE)
  { if(type & WHITE_OO)
    { PLM(F1, H1);
      square[H1] = WR;
      square[F1] = EMPTY_SQUARE;
    }
    if(type & WHITE_OOO)
    { PLM(D1, A1);
      square[A1] = WR;
      square[D1] = EMPTY_SQUARE;
    }
    if(type & BLACK_OO)
    { PLM(F8, H8);
      square[H8] = BR;
      square[F8] = EMPTY_SQUARE;
    }
    if(type & BLACK_OOO)
    { PLM(D8, A8);
      square[A8] = BR;
      square[D8] = EMPTY_SQUARE;
    }
  }
  
  if(type & PROM)
  { PLR(from);
    PLI(p,to);
    state->material -= pval[hs[hply].m.promoted];
    state->material_value[color] -= pval[hs[hply].m.promoted];
    state->piece_count[color]--;
    state->pawns++;
    Pawns(color)++;
    state->pawn_value[color] += P_VALUE;
    state->presence[color][GetType(hs[hply].m.promoted)]--;
    bitset(&board->bb_pawns[color], rsz[to]);
  }
  else PLM(from, to);
  square[to] = p;
  square[from] = EMPTY_SQUARE;

  if(GetType(p) == PAWN)
  { bitclear(&board->bb_pawns[color],rsz[from]);
    bitset(&board->bb_pawns[color],rsz[to]);
  }
  
  if(type & CAP)
  { if(type & EP)
    { if(color == WHITE) from -= 0x10;
      if(color == BLACK) from += 0x10;
    }
    PLI(hs[hply].captured,from);
    square[from] = hs[hply].captured;
    
    if(GetType(hs[hply].captured) == PAWN)
    { state->pawns++;
      Pawns(color^1)++;
      state->pawn_value[color ^ 1] += P_VALUE;
      bitset(&board->bb_pawns[color^1],rsz[from]);
    }
    else
    { state->material += pval[hs[hply].captured];
      state->material_value[color ^ 1] += pval[hs[hply].captured];
      state->piece_count[color ^ 1]++;
      state->presence[color ^ 1][GetType(hs[hply].captured)]++;
    }
  }
  
  board->en_passant = hs[hply].old_ep;
  if(GetType(p) != PAWN && (!hs[hply].captured)) board->mr50 --;
  if(hs[hply].flags) board->castle = hs[hply].flags;
  board->fullmove -= board->side; 
  board->hash = hs[hply].hash;
  board->phash = hs[hply].phash;
  return true;
}


bool move_make(move_t move)
{
  int color,hply,flags;
  uint64 hashkey,pawnkey;
  uint8 p,cap = 0;
  sq_t from, to, e_p_capture = 0;
  state_t *state = &board->state;
  
  hply = board->hply;
  from = move.from;
  to = move.to;
  color = board->side;
  p = PieceType(from);
  hashkey = board->hash;
  pawnkey = board->phash;
  flags = 0;

  if(move.type & CASTLE)
  { if(move.type & WHITE_OO)
    { if(is_in_check(WHITE) 
      || is_sq_attacked(F1, BLACK)
      || is_sq_attacked(G1, BLACK))
        return false;
      PLM(H1, F1);
      square[F1] = WR;
      square[H1] = EMPTY_SQUARE;
      hashkey ^= zobrist_psq[WHITE_ROOK][H1];
      hashkey ^= zobrist_psq[WHITE_ROOK][F1];
    }
    if(move.type & WHITE_OOO)
    { if(is_in_check(WHITE) 
      || is_sq_attacked(D1, BLACK)
      || is_sq_attacked(C1, BLACK))
        return false;
      PLM(A1, D1);
      square[D1] = WR;
      square[A1] = EMPTY_SQUARE;
      hashkey ^= zobrist_psq[WHITE_ROOK][A1];
      hashkey ^= zobrist_psq[WHITE_ROOK][D1];
    }
    if(move.type & BLACK_OO)
    { if(is_in_check(BLACK) 
      || is_sq_attacked(F8, WHITE)
      || is_sq_attacked(G8, WHITE))
        return false;
      PLM(H8, F8);
      square[F8] = BR;
      square[H8] = EMPTY_SQUARE;
      hashkey ^= zobrist_psq[BLACK_ROOK][H8];
      hashkey ^= zobrist_psq[BLACK_ROOK][F8];
    }
    if(move.type & BLACK_OOO)
    { if(is_in_check(BLACK) 
      || is_sq_attacked(D8, WHITE)
      || is_sq_attacked(C8, WHITE))
        return false;
      PLM(A8, D8);
      square[D8] = BR;
      square[A8] = EMPTY_SQUARE;
      hashkey ^= zobrist_psq[BLACK_ROOK][A8];
      hashkey ^= zobrist_psq[BLACK_ROOK][D8];
    }
  }
  
  if(move.type & CAP)
  { if(move.type & EP)
    { if(color == WHITE) e_p_capture = to - 0x10;
      if(color == BLACK) e_p_capture = to + 0x10;
      to = e_p_capture;
    }
    cap = PieceType(to);
    
    if(GetType(cap) == PAWN)
    { pawnkey ^= zobrist_psq[cap][to];
      state->pawns--;
      state->pawn_value[color ^ 1] -= P_VALUE;
      Pawns(color ^ 1)--;
      bitclear(&board->bb_pawns[color^1], rsz[to]);
    }
    else
    { state->material -= pval[cap];
      state->material_value[color ^ 1] -= pval[cap];
      state->piece_count[color ^ 1]--;
      state->presence[color ^ 1][GetType(cap)]--;
    }
    hashkey ^= zobrist_psq[cap][to];
    PLR(to);
    square[to] = EMPTY_SQUARE;
    to = move.to;
  }
  
  PLM(from, to);
  square[to] = p;
  square[from] = EMPTY_SQUARE;
  hashkey ^= zobrist_psq[p][from];
  hashkey ^= zobrist_ep[board->en_passant];	
  hs[hply].old_ep = board->en_passant;
  board->en_passant = 0; 
  if((p == WHITE_PAWN) && ((to - from) == 32)) board->en_passant = to - 16;
  if((p == BLACK_PAWN) && ((to - from) == -32))board->en_passant = to + 16;
  hashkey ^= zobrist_ep[board->en_passant];
  
  if(move.promoted)
  { PLR(to);
    PLI(move.promoted, to);
    square[to] = move.promoted;
    hashkey ^= zobrist_psq[move.promoted][to];
    pawnkey ^= zobrist_psq[p][from];
    state->pawns--;
    state->pawn_value[color] -= P_VALUE;
    Pawns(color)--;
    state->material += pval[move.promoted];
    state->material_value[color] += pval[move.promoted];
    state->piece_count[color]++;
    state->presence[color][GetType(move.promoted)]++;
    bitclear(&board->bb_pawns[color], rsz[from]);
  }
  else
  { hashkey ^= zobrist_psq[p][to];
    if(GetType(p) == PAWN)
    { pawnkey ^= zobrist_psq[p][from];
      pawnkey ^= zobrist_psq[p][to];
      bitclear(&board->bb_pawns[color], rsz[from]);
      bitset(&board->bb_pawns[color], rsz[to]);
    }
  }
    
  if(GetType(p) != PAWN && (!cap)) board->mr50 ++;
  else board->mr50 = 0;
  
  if(move.type & CASTLE)
  { flags = board->castle;
    board->castle &= ~((WHITE_OO | WHITE_OOO) << (color * 2));
  }
  else
  { if((GetType(p) == KING) && (board->castle != 0))
    { flags = board->castle;
      board->castle &= ~((WHITE_OO | WHITE_OOO) << (color * 2)) ;
    }
    if((GetType(p) == ROOK) && (board->castle != 0))
    { if((calc_rank(from)) == (RANK_8 * color))
      { switch(calc_file(from))
        { case FILE_A:
            flags = board->castle;
            board->castle &= ~((WHITE_OOO) << (color * 2));
            break;
          case FILE_H:
            flags = board->castle;
            board->castle &= ~((WHITE_OO) << (color * 2));
            break;
        }
      }
    }
  }
  
  board->fullmove += board->side;
  hs[hply].m.p = move.p;
  hs[hply].moved = p;
  hs[hply].flags = (uint8)flags;
  hs[hply].hash = board->hash;
  hs[hply].phash = board->phash;
  hs[hply].captured = cap;
  board->ply++;board->hply++;
  board->side ^= 1;board->xside ^= 1;
  if(flags)
  { hashkey ^= zobrist_castle[flags];
    hashkey ^= zobrist_castle[board->castle];
  }
  hashkey ^= zobrist_side[board->xside];
  hashkey ^= zobrist_side[board->side];

  board->hash = hashkey;
  board->phash = pawnkey;
  if(is_in_check(board->xside))
  { move_undo();
    return false;
  }
  return true;
}

void move_make_null(nullstack_t *ns)
{ ns->side = board->side;
  ns->xside = board->xside;
  ns->fifty = board->mr50;
  ns->flags = board->castle;
  ns->old_ep = board->en_passant;
  ns->hash = board->hash;
  board->en_passant = 0;
  board->mr50 = 0;
  board->side ^= 1;
  board->xside ^= 1;
  board->hash ^= zobrist_side[ns->side];
  board->hash ^= zobrist_side[ns->xside];
  board->hash ^= zobrist_ep[ns->old_ep];
  board->ply++;
}

void move_undo_null(nullstack_t *ns)
{ board->side = ns->side;
  board->xside = ns->xside;
  board->mr50 = (sint8)ns->fifty;
  board->castle = ns->flags;
  board->en_passant = ns->old_ep;
  board->hash = ns->hash;
  board->ply--;
}

bool make_if_legal(move_t m)
{ int i;
  int move_count; 
  bool found = 0; 
  int index = 0; 
  move_t ms[MOVE_STACK];
  
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
  if(found && move_make(ms[index])) return true;
  return false;
}

