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

void Init(void) {

  for (int sq = 0; sq < 64; sq++)
    castle_mask[sq] = 15;

  castle_mask[A1] = 13;
  castle_mask[E1] = 12;
  castle_mask[H1] = 14;
  castle_mask[A8] = 7;
  castle_mask[E8] = 3;
  castle_mask[H8] = 11;

  for (int tp = 0; tp < 12; tp++)
    for (int sq = 0; sq < 64; sq++)
      zob_piece[tp][sq] = Random64();

  for (int i = 0; i < 16; i++)
    zob_castle[i] = Random64();

  for (int i = 0; i < 8; i++)
    zob_ep[i] = Random64();
}
