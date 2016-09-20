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

uint64 hash_position()
{
  int i;
  uint64 key = 0;

  for(i = 0; i < 64; i++)
  { if(pos->square[i] != EMPTY)
    key ^= zobrist_psq[pos->square[i]][i];
  }
  key ^= zobrist_ep[pos->ep];
  key ^= zobrist_castle[pos->castle];
  if(pos->side == B) key ^= zobrist_btm;
  return key;
}

uint64 phash_position()
{
  int sq;
  bitboard_t pcs;
  uint64 key = 0;

  pcs = pos->occ[WP];
  while(pcs)
  { sq = bitscanf(pcs);
    bitclear(pcs, sq);
    key ^= zobrist_psq[WP][sq];
  }
  pcs = pos->occ[BP];
  while(pcs)
  { sq = bitscanf(pcs);
    bitclear(pcs, sq);
    key ^= zobrist_psq[BP][sq];
  }
  return key;
}

bool position_illegal()
{
  int ksqw, ksqb;
  if(popcnt(pos->occ[WK]) != 1) 
    return true;
  if(popcnt(pos->occ[BK]) != 1) 
    return true;
  if(popcnt(pos->occ[OCC_W] | pos->occ[OCC_B]) > 32)
    return true;
	
  #ifdef PROPER_POS_CASTLING
  /* illegal castle flags will just be ignored
     by the move generator. This option may
     be turned on only for explicitly legal
     castling flags
  */
  if(pos->square[E1] != WK)
  { if((pos->castle & WHITE_OO)
    || (pos->castle & WHITE_OOO))
      return true;
  }
  if(pos->square[E8] != BK)
  { if((pos->castle & BLACK_OO)
    || (pos->castle & BLACK_OOO))
      return true;
  }
  if(pos->square[A1] != WR
  && pos->castle&WHITE_OOO)
    return true;
  if(pos->square[H1] != WR
  && pos->castle&WHITE_OO)
    return true;
  if(pos->square[A8] != BR
  && pos->castle&BLACK_OOO)
    return true;
  if(pos->square[H8] != BR
  && pos->castle&BLACK_OO)
    return true;
  #endif
	
  ksqw = Ksq(W);
  ksqb = Ksq(B);
	
  if(is_sq_attacked_b(ksqw) && pos->side == B)
    return true;
  if(is_sq_attacked_w(ksqb) && pos->side == W)
    return true;
	
  if(k_moves[ksqw] & pos->occ[BK]) return true;
  if(k_moves[ksqb] & pos->occ[WK]) return true;
	
  return false;
}

static int char_to_piece(char c)
{	
  switch(c)
  { case 'p': return BP; break;
    case 'P': return WP; break;
    case 'r': return BR; break;
    case 'R': return WR; break;
    case 'n': return BN; break;
    case 'N': return WN; break;
    case 'b': return BB; break;
    case 'B': return WB; break;
    case 'q': return BQ; break;
    case 'Q': return WQ; break;
    case 'k': return BK; break;
    case 'K': return WK; break;
    case 0: return 0; break;
    default: break;
  }
  return 0;
}

bool position_set(char *fen)
{
  int i,j,sq,piece;
  int f = 0,r = 0;
  int fifty = 0;
	
  if(!fen || *fen == 0) return false;
	
  memset(pos, 0, sizeof(position_t));
	
  for(i = RANK_8; i >= RANK_1; i--)
  { for(j = FILE_A; j <= FILE_H; j++)
    { sq = CalcSq(j, i);
      piece = char_to_piece(*fen);
      if(piece)
      { if(piece == WK) pos->ksq[W] = sq;
        if(piece == BK) pos->ksq[B] = sq;
        pos->square[sq] = (uint8)piece;
        bitset(pos->occ[piece], sq);
        bitset(pos->occ[OCC_W + GetColor(piece)], sq);
        pos->pcount[piece]++;
      }
      else if(isdigit(*fen)) j += ((*(fen) - 0x30)-1);
      fen++;
      if(strchr(" /",*fen)) 
      { fen++;
        break;
      }
    }
  }
	
  if(*fen == 0) return false;
  while(*fen == ' ') fen++;
	
  switch(*fen)//side to move
  { case 'w': 
    case 'W': 
      pos->side =  W; 
    break;
    case 'b': 
    case 'B': 
      pos->side = B; 
    break;
    default: 
      return false; 
    break;
  }
  fen++;
	
  while(*fen == ' ') fen++;
  while(*fen != ' ')
  { switch(*fen)//castling abillity
    { case 'K' : pos->castle  |= WHITE_OO;  break;
      case 'k' : pos->castle  |= BLACK_OO;  break;
      case 'Q' : pos->castle  |= WHITE_OOO; break;
      case 'q' : pos->castle  |= BLACK_OOO; break;
      case '-' : break;
      default  : goto exit; break;
    }
    fen++;
  }
	
  while(*fen == ' ') fen++;
  while(*fen != ' ')
  { switch(*fen)//en passant square
    { case 'a' : f = FILE_A; break;
      case 'b' : f = FILE_B; break;
      case 'c' : f = FILE_C; break;
      case 'd' : f = FILE_D; break;
      case 'e' : f = FILE_E; break;
      case 'f' : f = FILE_F; break;
      case 'g' : f = FILE_G; break;
      case 'h' : f = FILE_H; break;
      case '1' : r = RANK_1; break;
      case '2' : r = RANK_2; break;
      case '3' : r = RANK_3; break;
      case '4' : r = RANK_4; break;
      case '5' : r = RANK_5; break;
      case '6' : r = RANK_6; break;
      case '7' : r = RANK_7; break;
      case '8' : r = RANK_8; break;
      case '-' : break;
      default  :goto exit; break;
    }
    fen++;
  }
  //calculating the en-passant square:
  pos->ep = (uint8)CalcSq(f, r);
	
  while(*fen == ' ') fen++;
  while(*fen != ' ')//getting the rule 50 halfmoves
  { if(*fen == 0) goto exit;
    if(isdigit(*fen))
    {//converting the num characters into integer
     //using local variable for safety reasons:
      sscanf(fen,"%d",&fifty);
      pos->mr50 = (sint8)fifty;
      if(pos->mr50 < 0) pos->mr50 = 0;
      if(isdigit(*(fen + 1))) fen++; //in case is 2 or more digit number
    }
    fen++;
  }
		
  while(*fen == ' ') fen++;
  if(*fen == 0) goto exit;
  //getting the fullmoves number at the end:
  sscanf(fen,"%d",&pos->fullmove);
  exit:
	
  pos->hash = hash_position();
  pos->phash = phash_position();
  pos->mhash = hash_material();
	
  if(position_illegal()) 
  { printf("illegal position!\n");
    position_set(INITIAL_POSITION);
    return false;
  }
  return true;
}


static char piece_to_char(int p)
{
  char c;
  switch(p)
  { case WP: c = 'P'; break;
    case BP: c = 'p'; break;
    case WK: c = 'K'; break;
    case BK: c = 'k'; break;
    case WQ: c = 'Q'; break;
    case BQ: c = 'q'; break;
    case WR: c = 'R'; break;
    case BR: c = 'r'; break;
    case WB: c = 'B'; break;
    case BB: c = 'b'; break;
    case WN: c = 'N'; break;
    case BN: c = 'n'; break;
    default: c =  0 ; break;
  }
  return c;
}

int position_get(char *fen)
{
  int sq,i,j,p;
  char *prank;
  char str_rank[8][8];
  int empty, c_count;
		
  if(!fen) return 0;
	
  //piece/empty squares tags:
  for(i = 7; i >= 0; i--)//rank
  { c_count = 0; empty = 0;
    prank = &str_rank[i][0];
    for(j = 0; j < 8; j ++)//file
    { sq = CalcSq(j,i);
      p = pos->square[sq];
      if(p == EMPTY) empty++;
      else
      { if(empty > 0)
        {//converting to a char
          *prank = (char)(empty + 0x30); 
          c_count++;
          prank++;
        }
        *prank = piece_to_char(p);
        c_count++;
        prank++;
        empty = 0;
      }
    }
    if(empty > 0)
    { *prank = (char)(empty + 0x30); 
      c_count ++;
    }
			
    //saving the current tag 
    strcpy(fen, (const char *)&str_rank[i][0]);
		
    //if it's not the last one (rank 1) - set last char as delimiter:
    if(i)
    { *(fen + c_count) = '/';
      fen += c_count + 1;
    }
    else //set it to space:
    { *(fen + c_count) = ' ';
      fen += c_count + 1;
    }
  }//the piece tags are over.
		
  //setting the side to move:
  if(pos->side == WHITE) *fen = 'w';
  else *fen = 'b';
  *(fen + 1) = ' ';
  fen+=2;
	
  //castling:
  if(pos->castle & WHITE_OO)
  { *fen = 'K';
    fen ++;
  }
  if(pos->castle & WHITE_OOO)
  { *fen = 'Q';
    fen ++;
  }
  if(pos->castle & BLACK_OO)
  { *fen = 'k';
    fen ++;
  }
  if(pos->castle & BLACK_OOO)
  { *fen = 'q';
    fen ++;
  }
  if(pos->castle == 0)
  { *fen = '-';
    fen++;
  }
  *fen = ' ';
  fen++;
		
  //ep square:
  if(pos->ep)
  { *fen = File(pos->ep) + 0x61; //convert to char
    *(fen + 1) = Rank(pos->ep) + 0x31;
    fen += 2;
  }
  else
  { *fen = '-';
    fen++;
  }
  *fen = ' ';
  fen++;
		
  //50 move halfclock
  fen += sprintf(fen,"%d",pos->mr50);
  *fen = ' ';
  fen++;
	
  //full move counter:
  fen += sprintf(fen,"%d",pos->fullmove);
	
  //null terminating:
  *fen = 0;
  return strlen(fen);
}
