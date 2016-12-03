/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rodent.h"
#include "eval.h"

void cEval::ScorePatterns(POS * p, eData *e) {

  U64 king_mask, rook_mask;

  // piece masks offer a minor speedup: pattern eval, using a lot of if statements,
  // is activated only when there is a piece that might constitute a part of the pattern.

  static const U64 wb_mask = {SqBb(F1) | SqBb(C1) | SqBb(A7) | SqBb(B8) | SqBb(H7) | SqBb(G8) | SqBb(A6) | SqBb(H6) | SqBb(B2) | SqBb(G2) };
  static const U64 bb_mask = {SqBb(F8) | SqBb(C8) | SqBb(A2) | SqBb(B1) | SqBb(H2) | SqBb(G1) | SqBb(A3) | SqBb(H3) | SqBb(B7) | SqBb(G7) };
  static const U64 wn_mask = {SqBb(A7) | SqBb(H7) };
  static const U64 bn_mask = {SqBb(A2) | SqBb(H2) };

  // White bishop patterns

  if (p->Bishops(WC) & wb_mask) {

    // Blockage of a central pawn on its initial square

    if (IsOnSq(p, WC, P, D2) && IsOnSq(p, WC, B, C1)
    && OccBb(p) & SqBb(D3)) Add(e, WC, F_OTHERS, -50, 0);

    if (IsOnSq(p, WC, P, E2) && IsOnSq(p, WC, B, F1)
    && OccBb(p) & SqBb(E3)) Add(e, WC, F_OTHERS, -50, 0);

    // Trapped bishop

    if (IsOnSq(p, WC, B, A7) && IsOnSq(p, BC, P, B6)) Add(e, WC, F_OTHERS, -150);
    if (IsOnSq(p, WC, B, B8) && IsOnSq(p, BC, P, C7)) Add(e, WC, F_OTHERS, -150);
    if (IsOnSq(p, WC, B, H7) && IsOnSq(p, BC, P, G6)) Add(e, WC, F_OTHERS, -150);
    if (IsOnSq(p, WC, B, G8) && IsOnSq(p, BC, P, F7)) Add(e, WC, F_OTHERS, -150);
    if (IsOnSq(p, WC, B, A6) && IsOnSq(p, BC, P, B5)) Add(e, WC, F_OTHERS, -50);
    if (IsOnSq(p, WC, B, H6) && IsOnSq(p, BC, P, G5)) Add(e, WC, F_OTHERS, -50);

    // Fianchettoed bishop

    if (IsOnSq(p, WC, B, B2)) {
      if (IsOnSq(p, WC, P, C3)) Add(e, WC, F_OTHERS, -10, -20);
      if (IsOnSq(p, WC, P, B3) && (IsOnSq(p, WC, P, A2) || IsOnSq(p, WC, P, C2))) Add(e, WC, F_OTHERS,  4);
      if (IsOnSq(p, BC, P, D4) && (IsOnSq(p, BC, P, E5) || IsOnSq(p, BC, P, C5))) Add(e, WC, F_OTHERS, -20);
    }

    if (IsOnSq(p, WC, B, G2)) {
      if (IsOnSq(p, WC, P, F3)) Add(e, WC, F_OTHERS, -10, -20);
      if (IsOnSq(p, WC, P, G3) && (IsOnSq(p, WC, P, H2) || IsOnSq(p, WC, P, F2))) Add(e, WC, F_OTHERS,  4);
      if (IsOnSq(p, BC, P, E4) && (IsOnSq(p, BC, P, D5) || IsOnSq(p, BC, P, F5))) Add(e, WC, F_OTHERS, -20);
    }
  }

  // Black bishop patterns

  if (p->Bishops(BC) & bb_mask) {

    // Blockage of a central pawn on its initial square

    if (IsOnSq(p, BC, P, D7) && IsOnSq(p, BC, B, C8)
    && OccBb(p) & SqBb(D6)) Add(e, BC, F_OTHERS, -50, 0);

    if (IsOnSq(p, BC, P, E7) && IsOnSq(p, BC, B, F8)
    && OccBb(p) & SqBb(E6)) Add(e, BC, F_OTHERS, -50, 0);

    // Trapped bishop

    if (IsOnSq(p, BC, B, A2) && IsOnSq(p, WC, P, B3)) Add(e, BC, F_OTHERS, -150);
    if (IsOnSq(p, BC, B, B1) && IsOnSq(p, WC, P, C2)) Add(e, BC, F_OTHERS, -150);
    if (IsOnSq(p, BC, B, H2) && IsOnSq(p, WC, P, G3)) Add(e, BC, F_OTHERS, -150);
    if (IsOnSq(p, BC, B, G1) && IsOnSq(p, WC, P, F2)) Add(e, BC, F_OTHERS, -150);
    if (IsOnSq(p, BC, B, A3) && IsOnSq(p, WC, P, B4)) Add(e, BC, F_OTHERS, -50);
    if (IsOnSq(p, BC, B, H3) && IsOnSq(p, WC, P, G4)) Add(e, BC, F_OTHERS, -50);

    // Fianchettoed bishop

    if (IsOnSq(p, BC, B, B7)) { 
      if (IsOnSq(p, BC, P, C6)) Add(e, BC, F_OTHERS, -10, -20);
      if (IsOnSq(p, BC, P, B6) && (IsOnSq(p, BC, P, A7) || IsOnSq(p, BC, P, C7))) Add(e, BC, F_OTHERS,  4);
      if (IsOnSq(p, WC, P, D5) && (IsOnSq(p, WC, P, E4) || IsOnSq(p, WC, P, C4))) Add(e, BC, F_OTHERS, -20); 
    }
    if (IsOnSq(p, BC, B, G7)) {
      if (IsOnSq(p, BC, P, F6)) Add(e, BC, F_OTHERS, -10, -20);
      if (IsOnSq(p, BC, P, G6) && (IsOnSq(p, BC, P, H7) || IsOnSq(p, BC, P, G6))) Add(e, BC, F_OTHERS,  4);
      if (IsOnSq(p, WC, P, E5) && (IsOnSq(p, WC, P, D4) || IsOnSq(p, WC, P, F4))) Add(e, BC, F_OTHERS, -20);
    }

  }

  // Trapped knight

  if (p->Knights(WC) & wn_mask) {
    if (IsOnSq(p, WC, N, A7)) {
      if (IsOnSq(p, BC, P, A6)) Add(e, WC, F_OTHERS, -75);
      if (IsOnSq(p, BC, P, B7)) Add(e, WC, F_OTHERS, -75);
    }

    if (IsOnSq(p, WC, N, H7)) {
      if (IsOnSq(p, BC, P, H6)) Add(e, WC, F_OTHERS, -75);
      if (IsOnSq(p, BC, P, G7)) Add(e, WC, F_OTHERS, -75);
    }
  }

  if (p->Knights(BC) & bn_mask) {
    if (IsOnSq(p, BC, N, A2)) {
      if (IsOnSq(p, WC, P, A3)) Add(e, BC, F_OTHERS, -75);
      if (IsOnSq(p, WC, P, B2)) Add(e, BC, F_OTHERS, -75);
    }

    if (IsOnSq(p, BC, N, H2)) {
      if (IsOnSq(p, WC, P, H3)) Add(e, BC, F_OTHERS, -75);
      if (IsOnSq(p, WC, P, G2)) Add(e, BC, F_OTHERS, -75);
    }
  }
  
  // Rook blocked by uncastled king

  king_mask = SqBb(F1) | SqBb(G1);
  rook_mask = SqBb(G1) | SqBb(H1) | SqBb(H2);

  if ((p->Kings(WC) & king_mask)
  &&  (p->Rooks(WC) & rook_mask)) Add(e, WC, F_OTHERS, -50, 0);

  king_mask = SqBb(B1) | SqBb(C1);
  rook_mask = SqBb(A1) | SqBb(B1) | SqBb(A2);

  if ((p->Kings(WC) & king_mask)
  &&  (p->Rooks(WC) & rook_mask)) Add(e, WC, F_OTHERS, -50, 0);

  king_mask = SqBb(F8) | SqBb(G8);
  rook_mask = SqBb(G8) | SqBb(H8) | SqBb(H7);

  if ((p->Kings(BC) & king_mask)
  &&  (p->Rooks(BC) & rook_mask)) Add(e, BC, F_OTHERS, -50, 0);

  king_mask = SqBb(B8) | SqBb(C8);
  rook_mask = SqBb(C8) | SqBb(B8) | SqBb(B7);

  if ((p->Kings(BC) & king_mask)
  &&  (p->Rooks(BC) & rook_mask)) Add(e, BC, F_OTHERS, -50, 0);

  // penalty for a castled king that cannot escape upwards
  
  if (IsOnSq(p, WC, K, H1) && IsOnSq(p, WC, P, H2) && IsOnSq(p, WC, P, G2))
     Add(e, WC, F_OTHERS, -15);

  if (IsOnSq(p, WC, K, G1) && IsOnSq(p, WC, P, H2) && IsOnSq(p, WC, P, G2) && IsOnSq(p, WC, P, F2))
     Add(e, WC, F_OTHERS, -15);

  if (IsOnSq(p, BC, K, H8) && IsOnSq(p, BC, P, H7) && IsOnSq(p, BC, P, G7))
     Add(e, BC, F_OTHERS, -15);

  if (IsOnSq(p, BC, K, G8) && IsOnSq(p, BC, P, H7) && IsOnSq(p, BC, P, G7) && IsOnSq(p, BC, P, F7) )
     Add(e, BC, F_OTHERS, -15);

  // castling rights

  if (IsOnSq(p, WC, K, E1)) {
    if ((p->castle_flags & W_KS) || (p->castle_flags & W_QS)) Add(e, WC, F_OTHERS, 10, 0);
  }

  if (IsOnSq(p, BC, K, E8)) {
    if ((p->castle_flags & B_KS) || (p->castle_flags & B_QS)) Add(e, BC, F_OTHERS, 10, 0);
  }
}