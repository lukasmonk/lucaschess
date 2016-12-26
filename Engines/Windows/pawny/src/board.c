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

static void board_clear()
{///fills the board with the 'empty' index
///side to move and opposite are set to -1 to get reset
  int i;
  memset(board,0,sizeof(position_t));
  memset(squares,OUTSIDE_BOARD,BREP);
  for(i = A1; i <= H8; i++) if(!IsOutside(i)) square[i] = EMPTY_SQUARE;
  board->side = -1;
  board->xside = -1;
}

static void init_piecelists()
{  
  int sq,p;
  state_t *state = &board->state;

  //init the pieces startup links:
  memset(piece, 0, PLSIZE * sizeof(list_t));
  for(p = BK; p; p--)
  { 
    PLS(p) = (sq_t)(PLSIZE - p + 1);
    PLP(PLSIZE - p + 1) = (sq_t)(PLSIZE - p);
  }
  
  //insert pieces:
  for(sq = 0; sq < PLSIZE; sq++)
  { 
    if(IsEmpty(Square(sq)) || IsOutside(Square(sq))) continue;
    p = square[Square(sq)];
    PLI((uint8)p, (sq_t)Square(sq));
    
    //initializing pawns and material info:
    switch(p) 
    { 
      case WP:
        Pawns(W)++;
        state->pawns++;
        state->pawn_value[W] += P_VALUE;
        bitset(&board->bb_pawns[W], rsz[Square(sq)]);
        break;
      case BP:
        Pawns(B)++;
        state->pawns++;
        state->pawn_value[B] += P_VALUE;
        bitset(&board->bb_pawns[B], rsz[Square(sq)]);
        break;
      case WK: case BK: break;
      default:
        state->material += pval[p];
        state->material_value[GetColor(p)]+= pval[p];
        state->piece_count[GetColor(p)]++;
        state->presence[GetColor(p)][GetType(p)]++;
        break;
    }
  }
}

bool board_from_fen(char  *fen)
{
  int sq;//square counter
  int f = 0,r = 0;//file and rank
  int fifty = 0;
  
  //is it an empty str.
  if(*fen == 0) return false;

  //first clearing the board:
  board_clear();
  
  //next - collects the pieces on board 
  //and the empty squares from the string
  //until entire board is filled:
  for (sq = A8; sq !=H1 + 1;fen++)
  {
    if(IsOutside(sq) && (sq > A1)) sq -= 24; //1 rank down
    switch(*fen)
    { //pieces and empty squares:
      case 'p': square[sq] = BLACK_PAWN;   break;
      case 'P': square[sq] = WHITE_PAWN;   break;
      case 'r': square[sq] = BLACK_ROOK;   break;
      case 'R': square[sq] = WHITE_ROOK;   break;
      case 'n': square[sq] = BLACK_KNIGHT; break;
      case 'N': square[sq] = WHITE_KNIGHT; break;
      case 'b': square[sq] = BLACK_BISHOP; break;
      case 'B': square[sq] = WHITE_BISHOP; break;
      case 'q': square[sq] = BLACK_QUEEN;  break;
      case 'Q': square[sq] = WHITE_QUEEN;  break;
      case 'k': square[sq] = BLACK_KING;   break;
      case 'K': square[sq] = WHITE_KING;   break;
      case '/': break;
      case 0: return false; break;
      default: break;
    }
    
    //empty squares:
    //converting the digit and returning backward (sq - 1)
    //because otherwise 'sq' will be incremented twice
    if(isdigit(*fen)) sq = (sq - 1) + (*(fen) - 0x30); 
    if(strchr(" /",*fen) == 0) sq++;
  }
  
  //now the pieces tag is over
  //continue to examine the FEN string
  if(*fen == 0) return false;
  while(*fen == ' ') fen++; //NEXT TAG
  switch(*fen)
  { //side to move
    case 'w' : case 'W' : board->side = WHITE; board->xside = BLACK; break;
    case 'b' : case 'B' : board->side = BLACK; board->xside = WHITE; break;
    default  : return false; break;
  }	
  fen++;
  
  while(*fen == ' ') fen++;//NEXT TAG
  while(*fen != ' ')
  { switch(*fen)
    { //castling avaibillity
      case 'K' : board->castle  |= WHITE_OO;  break;
      case 'k' : board->castle  |= BLACK_OO;  break;
      case 'Q' : board->castle  |= WHITE_OOO; break;
      case 'q' : board->castle  |= BLACK_OOO; break;
      case '-' : break;
      default  : goto exit; break;
    }
    fen++;
  }

  while(*fen == ' ') fen++;//NEXT TAG
  while(*fen != ' ')
  { //en passant sqare
    switch(*fen)
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
  board->en_passant = (sq_t)calc_sq(f, r);
  
  while(*fen == ' ') fen++;//NEXT TAG
  while(*fen != ' ')
  { //getting the rule 50 halfmoves
    if(*fen == 0) goto exit;
    if(isdigit(*fen))
    { //converting the num characters into integer
      //using local variable for safety reasons:
      sscanf(fen,"%d",&fifty);
      board->mr50 = (sint8)fifty;
      if(board->mr50 < 0) board->mr50 = 0;
      if(isdigit(*(fen + 1))) fen++; //in case is 2 or more digit number
    }
    fen++;
  }

  while(*fen == ' ') fen++;//NEXT TAG
  if(*fen == 0) goto exit;
  //getting the fullmoves number at the end:
  sscanf(fen,"%d",&board->fullmove);
  exit:
  //set above information in the PL's:
  init_piecelists();
  hash_board(); //create the initial hash key
  return true;
}


static char piece_to_char(int p)
{ char c;
  switch(p)
  { case WHITE_PAWN:   c = 'P'; break;
    case BLACK_PAWN:   c = 'p'; break;
    case WHITE_KING:   c = 'K'; break;
    case BLACK_KING:   c = 'k'; break;
    case WHITE_QUEEN:  c = 'Q'; break;
    case BLACK_QUEEN:  c = 'q'; break;
    case WHITE_ROOK:   c = 'R'; break;
    case BLACK_ROOK:   c = 'r'; break;
    case WHITE_BISHOP: c = 'B'; break;
    case BLACK_BISHOP: c = 'b'; break;
    case WHITE_KNIGHT: c = 'N'; break;
    case BLACK_KNIGHT: c = 'n'; break;
    default: c = 0; break;
  }
  return c;
}

int board_to_fen(char *fen)
{
  int sq,i,j;
  char *str_rank[8]; //str container for every rank.
  char *prank;
  int p; //piece type
  int empty;
  int c_count;
  
  //allocating the rank strings
  for(i = 0; i < 8; i ++)
    str_rank[i] = (char *)malloc(9); //8 fields + null terminator

  //getting the square information first:
  for(i = 7; i >= 0; i--) //rank
  { c_count = 0;
    empty = 0;
    prank = str_rank[i];
    for(j = 0; j < 8; j ++) //file
    { sq = calc_sq(j,i);
      if(square[sq] == EMPTY_SQUARE) p = P_NONE;
      else p = PieceType(sq);

      if(p == P_NONE) empty++;
      else
      {	if(empty > 0)
        { *prank = (char)(empty + 0x30); //converting to a char
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
    { *prank = (char)(empty + 0x30); //converting to a char
      c_count ++;
    }

    //recording the current tag 
    strcpy(fen, (const char *)str_rank[i]);

    //if it's not the last one (rank 1) - set las char as delimiter:
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
  if(board->side == WHITE) *fen = 'w';
  else *fen = 'b';
  *(fen + 1) = ' ';
  fen+=2;
  //castling ability:
  if(board->castle & WHITE_OO)
  { *fen = 'K';
    fen ++;
  }
  if(board->castle & WHITE_OOO)
  { *fen = 'Q';
    fen ++;
  }
  if(board->castle & BLACK_OO)
  { *fen = 'k';
    fen ++;
  }
  if(board->castle & BLACK_OOO)
  { *fen = 'q';
    fen ++;
  }
  if(board->castle == 0)
  { *fen = '-';
    fen++;
  }
  //adding space:
  *fen = ' ';
  fen++;

  //ep square:
  if(board->en_passant)
  { *fen = calc_file(board->en_passant) + 0x61; //convert to char
    *(fen + 1) = calc_rank(board->en_passant) + 0x31;
    fen += 2;
  }
  else
  {  *fen = '-';
     fen++;
  }
  //adding space:
  *fen = ' ';
  fen++;
  
  //50 move halfclock
  if(board->mr50 < 0) board->mr50 = 0;//hack
  fen += sprintf(fen,"%d",board->mr50);
  //adding space:
  *fen = ' ';
  fen++;
  
  //full move counter:
  fen += sprintf(fen,"%d",board->fullmove);
  
  //null terminating:
  *fen = 0;

  //release memory:
  for(i = 0; i < 8; i ++)
    free(str_rank[i]);
  return 0;
}

void board_init()
{///sets startup position:
  //setting the initial game position:
  if(!board_from_fen(INITIAL_POSITION))
    printf ("Illegal position (FEN) !\n");
}


void init_distance()
{ int i,j,file[2],rank[2];
  
  memset(&distance_table,0,sizeof(distance_table));
  for(i = 0; i < 128; i++)
  { file[0] = calc_file(i);
    rank[0] = calc_rank(i);
    for(j = 0; j < 128; j++)
    { file[1] = calc_file(j);
      rank[1] = calc_rank(j);
      distance_table[i][j] = max((abs(rank[0]-rank[1])), (abs(file[0]-file[1])));
    }
  }
}

void init_direction()
{ int i,t;
  const int *d;
  
  memset(&direction,0,sizeof(direction));
  for(i = 0; i < 128; i++)
  { for(d = dir_vect[QUEEN]; *d != 0; d++)
    { for(t = (i + (*d));(!IsOutside(t)); t += (*d))
        direction[i][t] = *d;
    }
  }
}



