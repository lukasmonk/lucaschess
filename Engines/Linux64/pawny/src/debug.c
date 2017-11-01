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

#define stm_eval(v) ((pos->side) ? (v) : (-v))

static const int flip[64] = 
{ A8, B8, C8, D8, E8, F8, G8, H8,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A4, B4, C4, D4, E4, F4, G4, H4,   
  A3, B3, C3, D3, E3, F3, G3, H3,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A1, B1, C1, D1, E1, F1, G1, H1
};

void debug_psq()
{ int i;
  for(i = 0; i < 64; i++)
  {
    if(psq_outposts_mg[W][i] != psq_outposts_mg[B][flip[i]])
      printf("error psq_outposts_mg\n");
    
    if(psq_outposts_eg[W][i] != psq_outposts_mg[B][flip[i]])
      printf("error psq_outposts_eg\n");
    
    if(psq_pawn_mg[W][i] != psq_pawn_mg[B][flip[i]])
      printf("error psq_pawn_mg\n");
    
    if(psq_knight_mg[W][i] != psq_knight_mg[B][flip[i]])
      printf("error psq_knight_mg\n");
    
    if(psq_knight_eg[W][i] != psq_knight_eg[B][flip[i]])
      printf("error psq_knight_eg\n");
    
    if(psq_bishop_mg[W][i] != psq_bishop_mg[B][flip[i]])
      printf("error psq_bishop_mg\n");
    
    if(psq_bishop_eg[W][i] != psq_bishop_eg[B][flip[i]])
      printf("error psq_bishop_eg\n");
        
    if(psq_rook_mg[W][i] != psq_rook_mg[B][flip[i]])
      printf("error psq_rook_mg\n");
        
    if(psq_rook_eg[W][i] != psq_rook_eg[B][flip[i]])
      printf("error psq_rook_eg\n");
    
    if(psq_queen_mg[W][i] != psq_queen_mg[B][flip[i]])
      printf("error psq_queen_mg\n");
        
    if(psq_queen_eg[W][i] != psq_queen_eg[B][flip[i]])
      printf("error psq_queen_eg\n");
    
    if(psq_king_mg[W][i] != psq_king_mg[B][flip[i]])
      printf("error psq_king_mg\n");
    
    if(psq_king_eg[W][i] != psq_king_eg[B][flip[i]])
      printf("error psq_king_eg\n");
  }
}

int debug_eval()
{
  int i, sq, piece;
  eval_info_t ei_internal;
  uint8 castle = 0;
  bitboard_t t;
  int old_eval;
  int r = 0;
  
  //evaluate before switching:
  old_eval = eval();
  memcpy(&ei_internal, &ei, sizeof(eval_info_t));
  
  //cleaning up the board:
  memset(&pos->square, 0, sizeof(pos->square));
  
  //filling the board with the info from 
  //the occupancy bitboards,
  //while flipping the roles of the pieces:
  for(piece = WP; piece <= WK; piece++)
  { t = pos->occ[piece];
    while(t)
    { sq = bitscanf(t);
      bitclear(t, sq);
      i = GetType(piece);
      i = Coloured(i, BLACK);
      pos->square[flip[sq]] = (uint8)i;
    }
  }
  for(piece = BP; piece <= BK; piece++)
  { t = pos->occ[piece];
    while(t)
    { sq = bitscanf(t);
      bitclear(t, sq);
      i = GetType(piece);
      i = Coloured(i, WHITE);
      pos->square[flip[sq]] = (uint8)i;
    }
  }
  
  //flip EP square:
  if(pos->ep) pos->ep = (uint8)flip[pos->ep];
  
  //flip side to move:
  pos->side ^= 1;
  
  //reversing castling rights:
  if(pos->castle & WHITE_OO) castle |= BLACK_OO;
  if(pos->castle & BLACK_OO) castle |= WHITE_OO;
  if(pos->castle & WHITE_OOO) castle |= BLACK_OOO;
  if(pos->castle & BLACK_OOO) castle |= WHITE_OOO;
  pos->castle = castle;
  
  //clear old data:
  memset(&pos->occ, 0, sizeof(pos->occ));
  memset(&pos->pcount, 0, sizeof(pos->pcount));
  
  //new occupancy/piece count:
  for(i = 0; i < 64; i ++)
  { piece = pos->square[i];
    if(piece)
    { if(piece == WK) pos->ksq[W] = i;
      if(piece == BK) pos->ksq[B] = i;
      bitset(pos->occ[piece], i);
      bitset(pos->occ[OCC_W + GetColor(piece)], i);
      pos->pcount[piece]++;
    }
  }
  
  //final step:
  pos->hash  = hash_position();
  pos->phash = phash_position();
  pos->mhash = mhash_position(); //not needed though
  
  //compare before/after role switching:
  ///reminder: returned eval() score is stm based, while parameters are zero-sum!
  if(stm_eval(old_eval) != stm_eval(eval())){
    printf("error - evaluation general: old = %d, new = %d\n", old_eval, eval());
    r = 1;
  }
  if(ei.material_mg != -ei_internal.material_mg)
    printf("error - material_mg: old = %d, new = %d\n", ei_internal.material_mg, ei.material_mg);
  if(ei.material_eg != -ei_internal.material_eg)
    printf("error - material_eg: old = %d, new = %d\n", ei_internal.material_eg, ei.material_eg);
  if(ei.pawn_structure_mg != -ei_internal.pawn_structure_mg)
    printf("error - pawn_structure_mg: old = %d, new = %d\n", ei_internal.pawn_structure_mg, ei.pawn_structure_mg);
  if(ei.pawn_structure_eg != -ei_internal.pawn_structure_eg)
    printf("error - pawn_structure_eg: old = %d, new = %d\n", ei_internal.pawn_structure_eg, ei.pawn_structure_eg);
  if(ei.mobility_mg != -ei_internal.mobility_mg)
    printf("error - mobility_mg: old = %d, new = %d\n", ei_internal.mobility_mg, ei.mobility_mg);
  if(ei.mobility_eg != -ei_internal.mobility_eg)
    printf("error - mobility_eg: old = %d, new = %d\n", ei_internal.mobility_eg, ei.mobility_eg);
  if(ei.psq_tables_mg != -ei_internal.psq_tables_mg)
    printf("error - psq_tables_mg: old = %d, new = %d\n", ei_internal.psq_tables_mg, ei.psq_tables_mg); 
  if(ei.psq_tables_eg != -ei_internal.psq_tables_eg)
    printf("error - psq_tables_eg: old = %d, new = %d\n", ei_internal.psq_tables_eg, ei.psq_tables_eg);
  if(ei.outposts_mg != -ei_internal.outposts_mg)
    printf("error - outposts_mg: old = %d, new = %d\n", ei_internal.outposts_mg, ei.outposts_mg);
  if(ei.outposts_eg != -ei_internal.outposts_eg)
    printf("error - outposts_eg: old = %d, new = %d\n", ei_internal.outposts_eg, ei.outposts_eg);
  if(ei.threats_mg != -ei_internal.threats_mg)
    printf("error - threats_mg: old = %d, new = %d\n", ei_internal.threats_mg, ei.threats_mg);
  if(ei.threats_eg != -ei_internal.threats_eg)
    printf("error - threats_eg: old = %d, new = %d\n", ei_internal.threats_eg, ei.threats_eg);
  if(ei.development_mg != -ei_internal.development_mg)
    printf("error - development_mg: old = %d, new = %d\n", ei_internal.development_mg, ei.development_mg);
  if(ei.placement_mg != -ei_internal.placement_mg)
    printf("error - placement_mg: old = %d, new = %d\n", ei_internal.placement_mg, ei.placement_mg);
  if(ei.placement_eg != -ei_internal.placement_eg)
    printf("error - placement_eg: old = %d, new = %d\n", ei_internal.placement_eg, ei.placement_eg);
  if(ei.king_safety_mg != -ei_internal.king_safety_mg)
    printf("error - king_safety_mg: old = %d, new = %d\n", ei_internal.king_safety_mg, ei.king_safety_mg);
  if(ei.passed_pawns_eg != -ei_internal.passed_pawns_eg)
    printf("error - passed_pawns_eg: old = %d, new = %d\n", ei_internal.passed_pawns_eg, ei.passed_pawns_eg);
  
  /// note: The global position structure remained flipped!
  ///       We assume further inspection if needed:
  ///       ('eval', 'board' commands etc).
  return r;
}

void debug()
{ /// Operates by hand, combined with:
  /// 'setboard', 'getfen', 'eval' and 'debug' commands
  debug_psq();
  if(!debug_eval())
    printf("\nNo evaluation errors.\n");
}

int debugfen(char *filename)
{///Works with fen files
  
  FILE *f;
  char *p;
  char line[255];
  char buff[255];
  int x, position = 0, errors = 0;
  
  f = fopen(filename, "r");
  if(f == NULL) return 0;
  
  while(fgets(&line[0], sizeof(line), f))
  { x = 0; 
    p = &line[0];
    while(*p != '\n' && *p != '\0')
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
    if(debug_eval()) errors++;    
  }
  printf("\n\nend_of_file.\n");
  printf("Positions with evaluation errors: %d\n\n", errors);
  fclose(f);
  return 1;
}
