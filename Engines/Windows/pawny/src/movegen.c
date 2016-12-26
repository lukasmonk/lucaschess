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


static void move_capture(move_t ms[],int *pcount,int from, int to)
{ 
  ms[*pcount].p = 0;
  ms[*pcount].score = 0;
  ms[*pcount].from = (sq_t)from;
  ms[*pcount].to = (sq_t)to;
  ms[*pcount].type = CAP;
  *pcount += 1;
}

static void move_ep(move_t ms[],int *pcount, int from, int to)
{	
  ms[*pcount].p = 0;
  ms[*pcount].score = 0;
  ms[*pcount].from = (sq_t)from;
  ms[*pcount].to = (sq_t)to;
  ms[*pcount].type = EP | CAP;
  *pcount += 1;
}

static void move_push(move_t ms[],int *pcount, int from, int to,int castle_flag)
{
  ms[*pcount].p = 0;
  ms[*pcount].score = 0;
  ms[*pcount].from = (sq_t)from;
  ms[*pcount].to = (sq_t)to;
  if(castle_flag) ms[*pcount].type = (uint8)(CASTLE | castle_flag);
  *pcount += 1;
}

static void move_promote(move_t ms[],int *pcount,int from, int to)
{
  int i;
  for(i = QUEEN; i >= KNIGHT; i--)
  { ms[*pcount].p = 0;
    ms[*pcount].score = 0;
    ms[*pcount].from = (sq_t)from;
    ms[*pcount].to = (sq_t)to;
    ms[*pcount].promoted = (uint8)Coloured(i);
    ms[*pcount].type = PROM;
    if(!IsEmpty(to)) ms[*pcount].type |= CAP;
    *pcount += 1;
  }
}

int move_gen(move_t ms[])
{
  const int *dir;
  int rank;
  int from,to;
  int color = board->side;
  int opcolor = board->xside;
  int count = 0;
  
  //pawn moves:
  for(from = PLS(Coloured(PAWN)); from <= H8; from = PLN(from))
  { if(color == WHITE)
    { rank = calc_rank(from);
      to = from + 15;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from + 17;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from + 16;
      if(IsEmpty(to))	
      { if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
        else move_push(&ms[0],&count, from, to, 0);
        to += 16;
        if((rank == RANK_2) && IsEmpty(to)) 
          move_push(&ms[0],&count, from, to, 0);
      }
    }
    else
    { rank = calc_rank(from);
      to = from - 15;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_2) move_promote(&ms[0],&count,  from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from - 17;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_2) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from - 16;
      if(IsEmpty(to))	
      { if(rank == RANK_2) move_promote(&ms[0],&count, from, to);
        else move_push(&ms[0],&count, from, to, 0);
        to -= 16;
        if((rank == RANK_7) && IsEmpty(to)) 
          move_push(&ms[0],&count, from, to, 0);
      }
    }
  }
  //queen moves:
  for(from = PLS(Coloured(QUEEN)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[QUEEN]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir)
        move_push(&ms[0],&count, from, to, 0);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //rooks:
  for(from = PLS(Coloured(ROOK)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[ROOK]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir)
        move_push(&ms[0],&count, from, to, 0);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //bishops
  for(from = PLS(Coloured(BISHOP)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[BISHOP]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir)
        move_push(&ms[0],&count, from, to, 0);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //knights
  for(from = PLS(Coloured(KNIGHT)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[KNIGHT]; *dir != 0; dir++)
    { to = from + *dir;
      if(IsEmpty(to)) move_push(&ms[0],&count, from, to, 0);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //king:
  from = PLS(Coloured(KING));
  for(dir = dir_vect[KING]; *dir != 0; dir++)
  { to = from + *dir;
    if(IsEmpty(to)) move_push(&ms[0],&count, from, to, 0);
    if(PieceColor(to) == opcolor)
      move_capture(&ms[0],&count, from, to);
  }
  //castling
  if(color == WHITE)
  { if(board->castle & WHITE_OO)
      if(IsEmpty(F1) && IsEmpty(G1) 
      && (PieceType(H1) == WHITE_ROOK))
        move_push(&ms[0],&count, E1, G1, WHITE_OO);

    if(board->castle & WHITE_OOO)
      if(IsEmpty(D1) && IsEmpty(C1) 
      && IsEmpty(B1) && (PieceType(A1) == WHITE_ROOK))
        move_push(&ms[0],&count, E1, C1, WHITE_OOO);
  }
  else//black to move:
  { if(board->castle & BLACK_OO)
      if(IsEmpty(F8) && IsEmpty(G8) 
      && (PieceType(H8) == BLACK_ROOK))
        move_push(&ms[0],&count, E8, G8, BLACK_OO);

    if(board->castle & BLACK_OOO) 
      if(IsEmpty(D8) && IsEmpty(C8) 
      && IsEmpty(B8) && (PieceType(A8) == BLACK_ROOK))
        move_push(&ms[0],&count, E8, C8, BLACK_OOO);
  }
  //en-passant
  if(board->en_passant)
  { from = board->en_passant + dir_vect[OpColoured(PAWN)][0];
    if(PieceType(from) == Coloured(PAWN))
      move_ep(&ms[0],&count, from, board->en_passant);
    from = board->en_passant + dir_vect[OpColoured(PAWN)][1];
    if(PieceType(from) == Coloured(PAWN))
      move_ep(&ms[0],&count, from, board->en_passant);
  }
  return count;
}


int move_gen_caps(move_t ms[])
{
  const int *dir;
  int from,to,rank;
  int color = board->side;
  int opcolor = board->xside;
  int count = 0;
  
  //pawn moves:
  for(from = PLS(Coloured(PAWN)); from <= H8; from = PLN(from))
  { if(color == WHITE)
    { rank = calc_rank(from);
      to = from + 15;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from + 17;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from + 16;
      if(IsEmpty(to))	
        if(rank == RANK_7) move_promote(&ms[0],&count, from, to);
    }
    else
    { rank = calc_rank(from);
      to = from - 15;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_2) move_promote(&ms[0],&count,  from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from - 17;
      if(PieceColor(to) == opcolor)
      { if(rank == RANK_2) move_promote(&ms[0],&count, from, to);
        else move_capture(&ms[0],&count, from, to);
      }
      to = from - 16;
      if(IsEmpty(to))	
        if(rank == RANK_2) move_promote(&ms[0],&count, from, to);
    }
  }
  //queen moves:
  for(from = PLS(Coloured(QUEEN)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[QUEEN]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //rooks:
  for(from = PLS(Coloured(ROOK)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[ROOK]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //bishops
  for(from = PLS(Coloured(BISHOP)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[BISHOP]; *dir != 0; dir++)
    { for(to = from + *dir; IsEmpty(to); to = to + *dir);
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //knights
  for(from = PLS(Coloured(KNIGHT)); from <= H8; from = PLN(from))
  { for(dir = dir_vect[KNIGHT]; *dir != 0; dir++)
    { to = from + *dir;
      if(PieceColor(to) == opcolor)
        move_capture(&ms[0],&count, from, to);
    }
  }
  //king:
  from = PLS(Coloured(KING));
  for(dir = dir_vect[KING]; *dir != 0; dir++)
  { to = from + *dir;
    if(PieceColor(to) == opcolor)
      move_capture(&ms[0],&count, from, to);
  }
  //en-passant
  if(board->en_passant)
  { from = board->en_passant + dir_vect[OpColoured(PAWN)][0];
    if(PieceType(from) == Coloured(PAWN))
      move_ep(&ms[0],&count, from, board->en_passant);
    from = board->en_passant + dir_vect[OpColoured(PAWN)][1];
    if(PieceType(from) == Coloured(PAWN))
      move_ep(&ms[0],&count, from, board->en_passant);
  }
  return count;

}

int move_gen_legal(move_t ms[])
{///returns legal moves only, 
///temporary (and very expensive) solution for the single reply ext.!
  int i,count;
  int j = 0;
  move_t temp_ms[MOVE_STACK];
  
    count = move_gen(&temp_ms[0]);
    for(i = 0; i < count; i++)
    {
      if(!move_make(temp_ms[i])) continue;
      move_undo();
      ms[j] = temp_ms[i];
      j++;
    }
    return j;
}


static bool is_between(int x, int y, int sq, int dir)
{//is the square 'sq' is between x and y.
  int i;
  for(i = x + dir; i != y; i += dir)
  { if(i == sq) return true;
  }
  return false;
}

static bool is_legal(int from, int to, int king_sq, int byside)
{//Simulating the move and test for legality.
//In the generate evasions, 'is_legal' is used to detect discovery checks.
  int swapped;
  bool attacked;
  
  if(IsEmpty(to))
  { PieceType(to) = PieceType(from);
    PieceType(from) = EMPTY_SQUARE;
    PLM((sq_t)from, (sq_t)to);
    //legality test and push to stack:
    attacked = is_sq_attacked((sq_t)king_sq, byside);
    //undo the simulated move:
    PieceType(from) = PieceType(to);
    PieceType(to) = EMPTY_SQUARE;
    PLM((sq_t)to, (sq_t)from);
    if(attacked) return false;
  }
  else //capture:
  { swapped = PieceType(to);
    PieceType(to) = PieceType(from);
    PieceType(from) = EMPTY_SQUARE;
    PLR((sq_t)to);
    PLM((sq_t)from, (sq_t)to);
    //legality test and push to stack:
    attacked = is_sq_attacked((sq_t)king_sq, byside);
    //undo the simulated move:
    PieceType(from) = PieceType(to);
    PieceType(to) = (sq_t)swapped;
    PLM((sq_t)to, (sq_t)from);
    PLI((uint8)swapped,(sq_t)to);
    if(attacked) return false;
  }
  return true;
}

int move_gen_evasions(move_t ms[])
{//legal evasions.	
  const int *dir;     //pointer to direction table member
  int to,sq,delta;    //destination, piece square,attack direction
  int is_slider;      //the single attacker is a slider? If so, the attacked king could be covered by a friendly piece;
  int checker_sq = 0; //the attacker square (it's usable only when a single check is examined)
  int count = 0;      //legal evasions count
  int checkers = 0;   //checking pieces count (xside)
  int king_sq = King_Square(board->side); //king in check.
  int side = board->side; //attacked side
  int xside = board->xside; //attacking side.
  attack_t *a = &attack_vector[king_sq][0];
  
  ///find the obvious attackers of xside.
  //queens:
  for(sq = PLS(OpColoured(QUEEN)); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK || a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(to = sq + delta; IsEmpty(to); to += delta);
      if(to == king_sq)
      { checker_sq = sq;
        checkers++;
      }
    }
  }
  //rooks:
  for(sq = PLS(OpColoured(ROOK)); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK)
    { delta = a[sq].delta;
      for(to = sq + delta; IsEmpty(to); to += delta);
      if(to == king_sq)
      { checker_sq = sq;
        checkers++;
      }
    }	
  }
  //bishops:
  for(sq = PLS(OpColoured(BISHOP)); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(to = sq + delta; IsEmpty(to); to += delta);
      if(to == king_sq)
      { checker_sq = sq;
        checkers++;
      }
    }	
  }
  //knights:
  for(sq = PLS(OpColoured(KNIGHT)); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & N_ATK)
    { if(sq + a[sq].delta == king_sq)
      { checker_sq = sq;
        checkers++;
      }
    }
  }
  //pawns:
  if(PieceType(king_sq + pawn_delta[side][0]) == OpColoured(PAWN))
  { checker_sq = king_sq + pawn_delta[side][0];
    checkers++;
  }
  if(PieceType(king_sq + pawn_delta[side][1]) == OpColoured(PAWN))
  { checker_sq = king_sq + pawn_delta[side][1];
    checkers++;
  }
  ///if it's not double check (at least)
  ///step 1.try to capture the attacker by any of the attacked side pieces:
  ///step 2.try to cover the king by any of the attacked side pieces:
  a = &attack_vector[checker_sq][0];
  if(checkers == 1) 
  { is_slider = slider[PieceType(checker_sq)];
    //queens:
    for(sq = PLS(Coloured(QUEEN)); sq <= H8; sq = PLN(sq))
    { //'capture the attacker' test:
      if(a[sq].flags & R_ATK || a[sq].flags & B_ATK)
      { delta = a[sq].delta;
        for(to = sq + delta; IsEmpty(to); to += delta);
        if((to == checker_sq) &&  is_legal(sq, to, king_sq, xside))
          move_capture(&ms[0],&count, sq, to);
      }
      if(is_slider)
      { //'cover the king' test:
        for(dir = dir_vect[QUEEN]; *dir != 0; dir++)
        { for(to = sq + *dir; IsEmpty(to); to += *dir)
          { if(direction[checker_sq][to] != 0)
            { if(direction[checker_sq][to] == \
              direction[checker_sq][king_sq])
              { if(is_between(checker_sq, king_sq, \
                to, direction[checker_sq][king_sq])
                && is_legal(sq, to, king_sq, xside))
                  move_push(&ms[0],&count, sq, to, 0);
              }
            }
          }
        }
      }
    }
    //rooks:
    for(sq = PLS(Coloured(ROOK)); sq <= H8; sq = PLN(sq))
    { //'capture the attacker' test:
      if(a[sq].flags & R_ATK)
      { delta = a[sq].delta;
        for(to = sq + delta; IsEmpty(to); to += delta);
        if((to == checker_sq) && is_legal(sq, to, king_sq, xside))
          move_capture(&ms[0],&count, sq, to);
      }
      if(is_slider)
      { //'cover the king' test:
        for(dir = dir_vect[ROOK]; *dir != 0; dir++)
        { for(to = sq + *dir; IsEmpty(to); to += *dir)
          { if(direction[checker_sq][to] != 0)
            { if(direction[checker_sq][to] == \
              direction[checker_sq][king_sq])
              { if(is_between(checker_sq, king_sq, \
                to, direction[checker_sq][king_sq])
                && is_legal(sq, to, king_sq, xside))
                  move_push(&ms[0],&count, sq, to, 0);
              }
            }
          }
        }
      }
    }
    //bishops:
    for(sq = PLS(Coloured(BISHOP)); sq <= H8; sq = PLN(sq))
    { //'capture the attacker' test:
      if(a[sq].flags & B_ATK)
      { delta = a[sq].delta;
        for(to = sq + delta; IsEmpty(to); to += delta);
        if((to == checker_sq) && is_legal(sq, to, king_sq, xside))
          move_capture(&ms[0],&count, sq, to);
      }	
      if(is_slider)
      { //'cover the king' test:
        for(dir = dir_vect[BISHOP]; *dir != 0; dir++)
        { for(to = sq + *dir; IsEmpty(to); to += *dir)
          { if(direction[checker_sq][to] != 0)
            { if(direction[checker_sq][to] == \
              direction[checker_sq][king_sq])
              { if(is_between(checker_sq, king_sq, \
                to, direction[checker_sq][king_sq])
                && is_legal(sq, to, king_sq, xside))
                  move_push(&ms[0],&count, sq, to, 0);
              }
            }
          }
        }
      }
    }
    //knights:
    for(sq = PLS(Coloured(KNIGHT)); sq <= H8; sq = PLN(sq))
    { //'capture the attacker' test:
      if(a[sq].flags & N_ATK)
      { to = sq + a[sq].delta;
        if((to == checker_sq) && is_legal(sq, to, king_sq, xside))
          move_capture(&ms[0],&count, sq, to);
      }
      if(is_slider)
      { //'cover the king' test:
        for(dir = dir_vect[KNIGHT]; *dir != 0; dir++)
        { to = sq + *dir;
          if(direction[checker_sq][to] != 0)
          { if(direction[checker_sq][to] == \
            direction[checker_sq][king_sq])
            { if(is_between(checker_sq, king_sq, \
              to, direction[checker_sq][king_sq])
              && is_legal(sq, to, king_sq, xside))
                move_push(&ms[0],&count, sq, to, 0);
            }
          }
        }
      }
    }
    //pawns:
    //'capture the attacker' test:
    sq = checker_sq + pawn_delta[xside][0];
    if(PieceType(sq) == Coloured(PAWN))
    { if(is_legal(sq, checker_sq, king_sq, xside))
      { if(calc_rank(sq) == pawn_prom_rank[side])//promotion + capture
          move_promote(&ms[0],&count, sq, checker_sq);
        else move_capture(&ms[0],&count, sq, checker_sq);//capture
      }
    }
    sq = checker_sq + pawn_delta[xside][1];
    if(PieceType(sq) == Coloured(PAWN))
    { if(is_legal(sq, checker_sq, king_sq, xside))
      { if(calc_rank(sq) == pawn_prom_rank[side])
          move_promote(&ms[0],&count, sq, checker_sq);
        else move_capture(&ms[0],&count, sq, checker_sq);
      }
    }
    //is en passant captures the attacker?
    //Here discovery check test is not needed since if e.p. causes it,
    //it means that the previous position was illegal.
    if(board->en_passant)
    { sq = board->en_passant + pawn_delta[xside][0];
      if(PieceType(sq) == Coloured(PAWN))
        move_ep(&ms[0],&count, sq, board->en_passant);
      sq = board->en_passant + pawn_delta[xside][1];
      if(PieceType(sq) == Coloured(PAWN))
        move_ep(&ms[0],&count, sq, board->en_passant);
    }
  
    if(is_slider)//testing the two types of pawn push for covering the king:
    { for(sq = PLS(Coloured(PAWN)); sq <= H8; sq = PLN(sq))
      { to = sq + pawn_push[side][0]; //single push
        if(!IsEmpty(to)) continue;
        if(direction[checker_sq][to] != 0)
        { if(direction[checker_sq][to] == \
          direction[checker_sq][king_sq])
          { if(is_between(checker_sq, king_sq, \
            to, direction[checker_sq][king_sq])
            && is_legal(sq, to, king_sq, xside))
            { if(calc_rank(to) == pawn_prom_rank_[side])//promotion (last rank hack)
                move_promote(&ms[0],&count, sq, to);
              else move_push(&ms[0],&count, sq, to, 0);
            }
          }
        }
        if(calc_rank(sq) == pawn_dpush_rank[side])
        { to = sq + pawn_push[board->side][1]; //double push
          if(!IsEmpty(to)) continue;
          if(direction[checker_sq][to] != 0)
          { if(direction[checker_sq][to] == \
            direction[checker_sq][king_sq])
            { if(is_between(checker_sq, king_sq, \
              to, direction[checker_sq][king_sq])
              && is_legal(sq, to, king_sq, xside))
                move_push(&ms[0],&count, sq, to, 0);
            }
          }
        }
      }
    }
  }
  //king evasions+captures:
  for(dir = dir_vect[KING]; *dir != 0; dir++)
  { to = king_sq + *dir;
    if(IsEmpty(to) && is_legal(king_sq, to, to, xside)) 
      move_push(&ms[0],&count, king_sq, to, 0);
    if(PieceColor(to) == xside && is_legal(king_sq, to, to, xside))
      move_capture(&ms[0],&count, king_sq, to);
  }
  return count;
}


