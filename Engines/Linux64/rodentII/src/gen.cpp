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
#include <stdio.h>

int *GenerateCaptures(POS *p, int *list) {

  U64 bbPieces, bbMoves, bbEnemy;
  int from, to;
  int side = p->side;

  bbEnemy = p->cl_bb[Opp(side)];

  if (side == WC) {

    // White pawn promotions with capture

    bbMoves = ((p->Pawns(WC) & ~FILE_A_BB & RANK_7_BB) << 7) & p->cl_bb[BC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to - 7);
      *list++ = (R_PROM << 12) | (to << 6) | (to - 7);
      *list++ = (B_PROM << 12) | (to << 6) | (to - 7);
      *list++ = (N_PROM << 12) | (to << 6) | (to - 7);
    }

  // White pawn promotions with capture

    bbMoves = ((p->Pawns(WC) & ~FILE_H_BB & RANK_7_BB) << 9) & p->cl_bb[BC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to - 9);
      *list++ = (R_PROM << 12) | (to << 6) | (to - 9);
      *list++ = (B_PROM << 12) | (to << 6) | (to - 9);
      *list++ = (N_PROM << 12) | (to << 6) | (to - 9);
    }

  // White pawn promotions without capture

    bbMoves = ((p->Pawns(WC) & RANK_7_BB) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to - 8);
      *list++ = (R_PROM << 12) | (to << 6) | (to - 8);
      *list++ = (B_PROM << 12) | (to << 6) | (to - 8);
      *list++ = (N_PROM << 12) | (to << 6) | (to - 8);
    }

  // White pawn captures

    bbMoves = ((p->Pawns(WC) & ~FILE_A_BB & ~RANK_7_BB) << 7) & p->cl_bb[BC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 7);
    }

  // White pawn captures

    bbMoves = ((p->Pawns(WC) & ~FILE_H_BB & ~RANK_7_BB) << 9) & p->cl_bb[BC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 9);
    }

  // White en passant capture

    if ((to = p->ep_sq) != NO_SQ) {
      if (((p->Pawns(WC) & ~FILE_A_BB) << 7) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to - 7);
      if (((p->Pawns(WC) & ~FILE_H_BB) << 9) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to - 9);
    }
  } else {

    // Black pawn promotions with capture

    bbMoves = ((p->Pawns(BC) & ~FILE_A_BB & RANK_2_BB) >> 9) & p->cl_bb[WC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to + 9);
      *list++ = (R_PROM << 12) | (to << 6) | (to + 9);
      *list++ = (B_PROM << 12) | (to << 6) | (to + 9);
      *list++ = (N_PROM << 12) | (to << 6) | (to + 9);
    }

  // Black pawn promotions with capture

    bbMoves = ((p->Pawns(BC) & ~FILE_H_BB & RANK_2_BB) >> 7) & p->cl_bb[WC];
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to + 7);
      *list++ = (R_PROM << 12) | (to << 6) | (to + 7);
      *list++ = (B_PROM << 12) | (to << 6) | (to + 7);
      *list++ = (N_PROM << 12) | (to << 6) | (to + 7);
    }

  // Black pawn promotions

    bbMoves = ((p->Pawns(BC) & RANK_2_BB) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (Q_PROM << 12) | (to << 6) | (to + 8);
      *list++ = (R_PROM << 12) | (to << 6) | (to + 8);
      *list++ = (B_PROM << 12) | (to << 6) | (to + 8);
      *list++ = (N_PROM << 12) | (to << 6) | (to + 8);
    }

  // Black pawn captures, excluding promotions

    bbMoves = ((p->Pawns(BC) & ~FILE_A_BB & ~RANK_2_BB) >> 9) & bbEnemy;
    while (bbMoves) { 
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 9);
    }

  // Black pawn captures, excluding promotions

    bbMoves = ((p->Pawns(BC) & ~FILE_H_BB & ~RANK_2_BB) >> 7) & bbEnemy;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 7);
    }

  // Black en passant capture

    if ((to = p->ep_sq) != NO_SQ) {
      if (((p->Pawns(BC) & ~FILE_A_BB) >> 9) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to + 9);
      if (((p->Pawns(BC) & ~FILE_H_BB) >> 7) & SqBb(to))
        *list++ = (EP_CAP << 12) | (to << 6) | (to + 7);
    }
  }

  // Captures by knight

  bbPieces = p->Knights(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.KnightAttacks(from) & bbEnemy;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Captures by bishop

  bbPieces = p->Bishops(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.BishAttacks(OccBb(p), from) & bbEnemy;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Captures by rook

  bbPieces = p->Rooks(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.RookAttacks(OccBb(p), from) & bbEnemy;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Captures by queen

  bbPieces = p->Queens(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.QueenAttacks(OccBb(p), from) & bbEnemy;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Captures by king

  bbMoves = BB.KingAttacks(KingSq(p, side)) & bbEnemy;
  while (bbMoves) {
    to = BB.PopFirstBit(&bbMoves);
    *list++ = (to << 6) | KingSq(p, side);
  }
  return list;
}

int *GenerateQuiet(POS *p, int *list) {

  U64 bbPieces, bbMoves;
  int from, to;
  int side = p->side;

  if (side == WC) {

    // White short castle

    if ((p->castle_flags & W_KS) && !(OccBb(p) & (U64)0x0000000000000060))
      if (!Attacked(p, E1, BC) && !Attacked(p, F1, BC))
        *list++ = (CASTLE << 12) | (G1 << 6) | E1;

  // White long castle

    if ((p->castle_flags & W_QS) && !(OccBb(p) & (U64)0x000000000000000E))
      if (!Attacked(p, E1, BC) && !Attacked(p, D1, BC))
        *list++ = (CASTLE << 12) | (C1 << 6) | E1;

  // White double pawn moves

    bbMoves = ((((p->Pawns(WC) & RANK_2_BB) << 8) & UnoccBb(p)) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to - 16);
    }

  // White normal pawn moves

    bbMoves = ((p->Pawns(WC) & ~RANK_7_BB) << 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 8);
    }
  } else {

    // Black short castle

    if ((p->castle_flags & B_KS) && !(OccBb(p) & (U64)0x6000000000000000))
      if (!Attacked(p, E8, WC) && !Attacked(p, F8, WC))
        *list++ = (CASTLE << 12) | (G8 << 6) | E8;

    // Black long castle

    if ((p->castle_flags & B_QS) && !(OccBb(p) & (U64)0x0E00000000000000))
      if (!Attacked(p, E8, WC) && !Attacked(p, D8, WC))
        *list++ = (CASTLE << 12) | (C8 << 6) | E8;

    // Black double pawn moves

    bbMoves = ((((p->Pawns(BC) & RANK_7_BB) >> 8) & UnoccBb(p)) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to + 16);
    }

    // Black single pawn moves

    bbMoves = ((p->Pawns(BC) & ~RANK_2_BB) >> 8) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 8);
    }
  }

  // Knight moves

  bbPieces = p->Knights(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.KnightAttacks(from) & UnoccBb(p);
  while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Bishop moves

  bbPieces = p->Bishops(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.BishAttacks(OccBb(p), from) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Rook moves

  bbPieces = p->Rooks(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.RookAttacks(OccBb(p), from) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // Queen moves

  bbPieces = p->Queens(p->side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = BB.QueenAttacks(OccBb(p), from) & UnoccBb(p);
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  // King moves

  bbMoves = BB.KingAttacks(KingSq(p, side)) & UnoccBb(p);
  while (bbMoves) {
    to = BB.PopFirstBit(&bbMoves);
    *list++ = (to << 6) | KingSq(p, side);
  }

  return list;
}

int *GenerateQuietChecks(POS *p, int *list)
{
  U64 bbPieces, bbMoves;
  int ksq = KingSq(p, Opp(p->side));
  U64 bbKnightChk = BB.KnightAttacks(ksq);
  U64 bbStr8Chk = BB.RookAttacks(OccBb(p), ksq);
  U64 bbDiagChk = BB.BishAttacks(OccBb(p), ksq);
  U64 bbQueenChk = bbStr8Chk | bbDiagChk;
  U64 bbPawnChk = BB.ShiftFwd( BB.ShiftSideways(SqBb(ksq)), Opp(p->side));

  int side, from, to;

  side = p->side;
  int op = Opp(side);

  if (side == WC) {

    // White pawn checks by a double move

    bbMoves = ((((p->Pawns(WC) & RANK_2_BB) << 8) & UnoccBb(p)) << 8) & UnoccBb(p);
    bbMoves &= bbPawnChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to - 16);
    }

    // White pawn checks by a single move

    bbMoves = ((p->Pawns(WC) & ~RANK_7_BB) << 8) & UnoccBb(p);
    bbMoves &= bbPawnChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to - 8);
    }

  } else {

    // Black pawn checks by a double move

    bbMoves = ((((p->Pawns(BC) & RANK_7_BB) >> 8) & UnoccBb(p)) >> 8) & UnoccBb(p);
    bbMoves &= bbPawnChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (EP_SET << 12) | (to << 6) | (to + 16);
    }

    // Black pawn checks by a single move

    bbMoves = ((p->Pawns(BC) & ~RANK_2_BB) >> 8) & UnoccBb(p);
    bbMoves &= bbPawnChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | (to + 8);
    }
  }

  bbPieces = p->Knights(side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
	int knight_discovers = 0;

	U64 bbCheckers = p->Queens(op) | p->Rooks(op) | p->Bishops(op);
	while (bbCheckers) {
		int checker = BB.PopFirstBit(&bbCheckers);
		U64 bbRay = BB.bbBetween[checker][p->king_sq[op]];

		if (SqBb(from) & bbRay) {
			if (BB.PopCnt(bbRay & OccBb(p)) == 1) {
				knight_discovers = 1;
				break;
			}
		}
	}

	bbMoves = (BB.KnightAttacks(from) & UnoccBb(p));
	if (!knight_discovers) bbMoves &= bbKnightChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  bbPieces = p->Bishops(side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = (BB.BishAttacks(OccBb(p), from) & UnoccBb(p)) & bbDiagChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  bbPieces = p->Rooks(side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = (BB.RookAttacks(OccBb(p), from) & UnoccBb(p)) & bbStr8Chk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  bbPieces = p->Queens(side);
  while (bbPieces) {
    from = BB.PopFirstBit(&bbPieces);
    bbMoves = (BB.QueenAttacks(OccBb(p), from) & UnoccBb(p)) & bbQueenChk;
    while (bbMoves) {
      to = BB.PopFirstBit(&bbMoves);
      *list++ = (to << 6) | from;
    }
  }

  return list;
}