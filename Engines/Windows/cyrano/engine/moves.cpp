#include "common.h"

/*external variables*/
const int pawn_dir[2] = {UU,DD};
const int col_tab[15] = {neutral,white,white,white,white,white,white,
black,black,black,black,black,black,neutral};
const int pic_tab[15] = {empty,king,queen,rook,bishop,knight,pawn,
king,queen,rook,bishop,knight,pawn,elephant};
SQATTACK  temp_sqatt[0x101];
PSQATTACK const sqatt = &temp_sqatt[0x80];

/*
MOVES
*/

void SEARCHER::do_move(const int& move) {

	int from = m_from(move),to = m_to(move),sq;

	/*remove captured piece*/
	if(m_capture(move)) {
		if(is_ep(move)) {
			sq = to - pawn_dir[player];
		} else {
			sq = to;
		}
		pcRemove(m_capture(move),sq);
		board[sq] = empty;
	}

	/*move piece*/
	if(m_promote(move)) {
		board[to] = m_promote(move);
		board[from] = empty;
		pcAdd(m_promote(move),to);
		pcRemove(COMBINE(player,pawn),from);
	} else {
		board[to] = board[from];
		board[from] = empty;
		pcSwap(from,to);
	}

	/*move castle*/
	if(is_castle(move)) {
        int fromc,toc;
		if(to > from) {
           fromc = to + RR;
		   toc = to + LL;
		} else {
           fromc = to + 2*LL;
		   toc = to + RR;
		}
		board[toc] = board[fromc];
		board[fromc] = empty;
		pcSwap(fromc,toc);
	} 

	/*update current state*/
	epsquare = 0;
	fifty++;
	if(DECOMB(player,m_piece(move)) == pawn) {
		fifty = 0;
	    if(to - from == (2 * pawn_dir[player])) {
            epsquare = ((to + from) >> 1);
		}
	}else if(m_capture(move)) {
		fifty = 0;
	}
	int p_castle = castle;
	if(from == E1 || to == A1 || from == A1) castle &= ~WLC_FLAG;
	if(from == E1 || to == H1 || from == H1) castle &= ~WSC_FLAG;
	if(from == E8 || to == A8 || from == A8) castle &= ~BLC_FLAG;
	if(from == E8 || to == H8 || from == H8) castle &= ~BSC_FLAG;

	player = invert(player);
	opponent = invert(opponent);
}

void SEARCHER::undo_move(const int& move) {
	int to,from,sq;

	player = invert(player);
	opponent = invert(opponent);


	to = m_to(move);
	from = m_from(move);

	/*unmove castle*/
	if(is_castle(move)) {
        int fromc,toc;
		if(to > from) {
           fromc = to + LL;
		   toc = to + RR;
		} else {
           fromc = to + RR;
		   toc = to + 2*LL;
		}
		board[toc] = board[fromc];
		board[fromc] = empty;
		pcSwap(fromc,toc);
	} 

	/*unmove piece*/
	if(m_promote(move)) {
		board[from] = COMBINE(player,pawn);
		board[to] = empty;
		pcAdd(COMBINE(player,pawn),from);
		pcRemove(m_promote(move),to);

	} else {
		board[from] = board[to];
		board[to] = empty;
		pcSwap(to,from);
	}

	/*insert captured piece*/
	if(m_capture(move)) {
		if(is_ep(move)) {
			sq = to - pawn_dir[player];
		} else {
			sq = to;
		}
		board[sq] = m_capture(move);
		pcAdd(m_capture(move),sq);
	}
}

#define NK_CAP(dir) {\
	    to = from + dir;\
       	if(COLOR(board[to]) == opponent) \
			*pmove++ = tmove | (to<<8) | (board[to]<<20);\
};
#define BRQ_CAP(dir) {\
	    to = from + dir; \
		while(board[to] == empty) to += dir;\
		if(COLOR(board[to]) == opponent) \
			*pmove++ = tmove | (to<<8) | (board[to]<<20);\
};

void SEARCHER::gen_caps() {
	int* pmove = &pstack->move_st[pstack->count],*spmove = pmove;
	int  from,to,tmove;
	PLIST current;
	
	if(player == white) {
		/*pawns*/
		current = plist[wpawn];
		while(current) {
			from = current->sq;
			to = from + RU;
			if(COLOR(board[to]) == black) {
				if(rank(to) == RANK8) {
					tmove = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (wqueen<<24);
					*pmove++ = tmove | (wknight<<24);
					*pmove++ = tmove | (wrook<<24);
					*pmove++ = tmove | (wbishop<<24);
				} else 
					*pmove++ = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
			}
			to = from + LU;
			if(COLOR(board[to]) == black) {
				if(rank(to) == RANK8) {
					tmove = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (wqueen<<24);
					*pmove++ = tmove | (wknight<<24);
					*pmove++ = tmove | (wrook<<24);
					*pmove++ = tmove | (wbishop<<24);
				} else 
					*pmove++ = from | (to<<8) | (wpawn<<16) | (board[to]<<20);
			}

			to = from + UU;
			if(rank(to) == RANK8) {
				if(board[to] == empty)
					*pmove++ = from | (to<<8) | (wpawn<<16) | (wqueen<<24);
			}	
			current = current->next;
		}
	
		if(epsquare) {
			from = epsquare + LD;
			if(board[from] == wpawn)
				*pmove++ = from | (epsquare<<8) | (wpawn<<16) | (bpawn<<20) | EP_FLAG;
			
			from = epsquare + RD;
			if(board[from] == wpawn)
				*pmove++ = from | (epsquare<<8) | (wpawn<<16) | (bpawn<<20) | EP_FLAG;
		}
		/*knight*/
		current = plist[wknight];
		while(current) {
			from = current->sq;
			tmove = from | (wknight<<16);
			NK_CAP(RRU);
			NK_CAP(LLD);
			NK_CAP(RUU);
			NK_CAP(LDD);
			NK_CAP(LLU);
			NK_CAP(RRD);
			NK_CAP(RDD);
			NK_CAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			from = current->sq;
			tmove = from | (wbishop<<16);
			BRQ_CAP(RU);
			BRQ_CAP(LD);
			BRQ_CAP(LU);
			BRQ_CAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			from = current->sq;
			tmove = from | (wrook<<16);
			BRQ_CAP(UU);
			BRQ_CAP(DD);
			BRQ_CAP(RR);
			BRQ_CAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			from = current->sq;
			tmove = from | (wqueen<<16);
			BRQ_CAP(RU);
			BRQ_CAP(LD);
			BRQ_CAP(LU);
			BRQ_CAP(RD);
			BRQ_CAP(UU);
			BRQ_CAP(DD);
			BRQ_CAP(RR);
			BRQ_CAP(LL);
			current = current->next;
		}
		/*king*/
		from = plist[wking]->sq;
		tmove = from | (wking<<16);
		NK_CAP(RU);
		NK_CAP(LD);
		NK_CAP(LU);
		NK_CAP(RD);
		NK_CAP(UU);
		NK_CAP(DD);
		NK_CAP(RR);
		NK_CAP(LL);
	} else {
		/*pawns*/
		current = plist[bpawn];
		while(current) {
			from = current->sq;
			to = from + LD;
			if(COLOR(board[to]) == white) {
				if(rank(to) == RANK1) {
					tmove = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (bqueen<<24);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else 
					*pmove++ = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
			}
			to = from + RD;
			if(COLOR(board[to]) == white) {
				if(rank(to) == RANK1) {
					tmove = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
					*pmove++ = tmove | (bqueen<<24);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else 
					*pmove++ = from | (to<<8) | (bpawn<<16) | (board[to]<<20);
			}
			
			to = from + DD;
			if(rank(to) == RANK1) {
				if(board[to] == empty)
					*pmove++ = from | (to<<8) | (bpawn<<16) | (bqueen<<24);
			}	
			current = current->next;
		}

		if(epsquare) {
			from = epsquare + RU;
			if(board[from] == bpawn)
				*pmove++ = from | (epsquare<<8) | (bpawn<<16) | (wpawn<<20) | EP_FLAG;
			
			from = epsquare + LU;
			if(board[from] == bpawn)
				*pmove++ = from | (epsquare<<8) | (bpawn<<16) | (wpawn<<20) | EP_FLAG;
		}
		/*knight*/
		current = plist[bknight];
		while(current) {
			from = current->sq;
			tmove = from | (bknight<<16);
			NK_CAP(RRU);
			NK_CAP(LLD);
			NK_CAP(RUU);
			NK_CAP(LDD);
			NK_CAP(LLU);
			NK_CAP(RRD);
			NK_CAP(RDD);
			NK_CAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			from = current->sq;
			tmove = from | (bbishop<<16);
			BRQ_CAP(RU);
			BRQ_CAP(LD);
			BRQ_CAP(LU);
			BRQ_CAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			from = current->sq;
			tmove = from | (brook<<16);
			BRQ_CAP(UU);
			BRQ_CAP(DD);
			BRQ_CAP(RR);
			BRQ_CAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			from = current->sq;
			tmove = from | (bqueen<<16);
			BRQ_CAP(RU);
			BRQ_CAP(LD);
			BRQ_CAP(LU);
			BRQ_CAP(RD);
			BRQ_CAP(UU);
			BRQ_CAP(DD);
			BRQ_CAP(RR);
			BRQ_CAP(LL);
			current = current->next;
		}
		/*king*/
		from = plist[bking]->sq;
		tmove = from | (bking<<16);
		NK_CAP(RU);
		NK_CAP(LD);
		NK_CAP(LU);
		NK_CAP(RD);
		NK_CAP(UU);
		NK_CAP(DD);
		NK_CAP(RR);
		NK_CAP(LL);
	}
	/*count*/
	pstack->count += (pmove - spmove);
}

#define NK_NONCAP(dir) {\
		to = from + dir;\
		if(board[to] == empty)\
			*pmove++ = tmove | (to<<8); \
};
#define BRQ_NONCAP(dir) {\
	    to = from + dir; \
		while(board[to] == empty) {\
			*pmove++ = tmove | (to<<8); \
			to += dir;\
		}\
};

void SEARCHER::gen_noncaps() {
	int* pmove = &pstack->move_st[pstack->count],*spmove = pmove;
	int  from,to,tmove;
	PLIST current;
	
	if(player == white) {

		/*castling*/
		if(!attacks(black,E1)) {
			if(castle & WSC_FLAG &&
				board[F1] == empty &&
				board[G1] == empty &&
				!attacks(black,F1) &&
				!attacks(black,G1))
				*pmove++ = E1 | (G1<<8) | (wking<<16) | CASTLE_FLAG;
			if(castle & WLC_FLAG &&
				board[B1] == empty &&
				board[C1] == empty &&
				board[D1] == empty &&
				!attacks(black,C1) &&
				!attacks(black,D1)) {
				*pmove++ = E1 | (C1<<8) | (wking<<16) | CASTLE_FLAG;
			}
		}
		/*knight*/
		current = plist[wknight];
		while(current) {
			from = current->sq;
			tmove = from | (wknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			from = current->sq;
			tmove = from | (wbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			from = current->sq;
			tmove = from | (wrook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			from = current->sq;
			tmove = from | (wqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*king*/
		from = plist[wking]->sq;
		tmove = from | (wking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);
		
		/*pawn*/
		current = plist[wpawn];
		while(current) {
			from = current->sq;
			to = from + UU;
			if(board[to] == empty) {
				if(rank(to) == RANK8) {
					tmove = from | (to<<8) | (wpawn<<16);
					*pmove++ = tmove | (wknight<<24);
					*pmove++ = tmove | (wrook<<24);
					*pmove++ = tmove | (wbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (wpawn<<16);
					
					if(rank(from) == RANK2) {
						to += UU;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (wpawn<<16);
					}
				}
			}	
			current = current->next;
		}
	} else {

		/*castling*/
		if(!attacks(white,E8)) {
			if(castle & BSC_FLAG &&
				board[F8] == empty &&
				board[G8] == empty &&
				!attacks(white,F8) &&
				!attacks(white,G8))
				*pmove++ = E8 | (G8<<8) | (bking<<16) | CASTLE_FLAG;
			if(castle & BLC_FLAG &&
				board[B8] == empty &&
				board[C8] == empty &&
				board[D8] == empty &&
				!attacks(white,C8) &&
				!attacks(white,D8)) {
				*pmove++ = E8 | (C8<<8) | (bking<<16) | CASTLE_FLAG;
			}
		}

		/*knight*/
		current = plist[bknight];
		while(current) {
			from = current->sq;
			tmove = from | (bknight<<16);
			NK_NONCAP(RRU);
			NK_NONCAP(LLD);
			NK_NONCAP(RUU);
			NK_NONCAP(LDD);
			NK_NONCAP(LLU);
			NK_NONCAP(RRD);
			NK_NONCAP(RDD);
			NK_NONCAP(LUU);
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			from = current->sq;
			tmove = from | (bbishop<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			from = current->sq;
			tmove = from | (brook<<16);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			from = current->sq;
			tmove = from | (bqueen<<16);
			BRQ_NONCAP(RU);
			BRQ_NONCAP(LD);
			BRQ_NONCAP(LU);
			BRQ_NONCAP(RD);
			BRQ_NONCAP(UU);
			BRQ_NONCAP(DD);
			BRQ_NONCAP(RR);
			BRQ_NONCAP(LL);
			current = current->next;
		}
		
		/*king*/
		from = plist[bking]->sq;
		tmove = from | (bking<<16);
		NK_NONCAP(RU);
		NK_NONCAP(LD);
		NK_NONCAP(LU);
		NK_NONCAP(RD);
		NK_NONCAP(UU);
		NK_NONCAP(DD);
		NK_NONCAP(RR);
		NK_NONCAP(LL);
		
		/*pawn*/
		current = plist[bpawn];
		while(current) {
			from = current->sq;
			to = from + DD;
			if(board[to] == empty) {
				if(rank(to) == RANK1) {
					tmove = from | (to<<8) | (bpawn<<16);
					*pmove++ = tmove | (bknight<<24);
					*pmove++ = tmove | (brook<<24);
					*pmove++ = tmove | (bbishop<<24);
				} else {
					*pmove++ = from | (to<<8) | (bpawn<<16);
					
					if(rank(from) == RANK7) {
						to += DD;
						if(board[to] == empty)
							*pmove++ = from | (to<<8) | (bpawn<<16);
					}
				}
			}	
			current = current->next;
		}
	}
	/*count*/
	pstack->count += (pmove - spmove);
}

/*constructro*/
SEARCHER::SEARCHER() : board(&temp_board[48])
{
	int sq;
	for (sq = 0;sq < 128; sq++) {
		list[sq] = new LIST;
	}
	for(sq = 0;sq < 48;sq++)
		temp_board[sq] = elephant;
    for(sq = 176;sq < 224;sq++)
		temp_board[sq] = elephant;
	for(sq = A1;sq < A1 + 128;sq++) {
		if(sq & 0x88)
           board[sq] = elephant;
	}
	used = 0;
}

void SEARCHER::init_data() {
	register int i,sq,pic;

 	ply = 0;
	pstack = stack + 0;

	for(i = wking;i < elephant;i++) {
       plist[i] = 0;
	}

	for(sq = A1;sq <= H8;sq++) {
		if(!(sq & 0x88)) { 
			list[sq]->sq = sq;
			list[sq]->prev = 0;
			list[sq]->next = 0;
			pic = board[sq];
			if(pic != empty) {
				pcAdd(pic,sq);
			}
		}
	}
}

void SEARCHER::set_pos(int side,int w_ksq,int b_ksq,
			  int piece1, int square1, 
			  int piece2, int square2,
			  int piece3, int square3
			  ) {

    register int sq;
	for(sq = A1;sq <= H8;sq++) {
		if(!(sq & 0x88)) {
			board[sq] = empty;
		} else {
			sq += 0x07;
		}
	}

	board[w_ksq] = wking; 
	board[b_ksq] = bking; 
	if(piece1)
		board[square1] = piece1; 
	if(piece2)
        board[square2] = piece2; 
	if(piece3)
        board[square3] = piece3; 

	player = side;
	opponent = invert(side);
	castle = 0;
	epsquare = 0;
	fifty = 0;
	init_data();
}

/*initialize square attack table*/
void init_sqatt() {
	const int king_step[8] = {RR , LL , UU , DD , RU , LD , LU , RD};
    const int knight_step[8] = {RRU , LLD , LLU , RRD , RUU , LDD , LUU , RDD};
	int i,j;
    for(i = 0;i < 0x101; i++) {
		temp_sqatt[i].pieces = 0;
        temp_sqatt[i].step = 0;
	}
	sqatt[RU].pieces |= WPM;
    sqatt[LU].pieces |= WPM;
    sqatt[RD].pieces |= BPM;
    sqatt[LD].pieces |= BPM;
	for(i = 0;i < 8;i++) {
		sqatt[king_step[i]].pieces |= KM;
		sqatt[knight_step[i]].pieces |= NM;
		for(j = 1;j < 8; j++) {
			sqatt[king_step[i] * j].step = king_step[i];
			if(i < 4)
                sqatt[king_step[i] * j].pieces |= (RM | QM);
            else
				sqatt[king_step[i] * j].pieces |= (BM | QM);
		}
	}
}

/*any blocking piece in between?*/
int SEARCHER::blocked(int from, int to) {
	register int step,sq;
	if(step = sqatt[to - from].step) {
		sq = from + step;
		while(board[sq] == empty && (sq != to)) sq += step;
		return (sq != to);
	}
	return true;
};

/*is square attacked by color?*/
int SEARCHER::attacks(int col,int sq) {
	register PLIST current;
	
	if(col == white) {
		/*pawn*/
		if(board[sq + LD] == wpawn) return true;
        if(board[sq + RD] == wpawn) return true;
		/*knight*/
		current = plist[wknight];
		while(current) {
			if(sqatt[sq - current->sq].pieces & NM)
				return true;
			current = current->next;
		}
		/*bishop*/
		current = plist[wbishop];
		while(current) {
			if(sqatt[sq - current->sq].pieces & BM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*rook*/
		current = plist[wrook];
		while(current) {
			if(sqatt[sq - current->sq].pieces & RM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*queen*/
		current = plist[wqueen];
		while(current) {
			if(sqatt[sq - current->sq].pieces & QM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*king*/
		if(sqatt[sq - plist[wking]->sq].pieces & KM)
			return true;
	} else if(col == black) {
		/*pawn*/
		if(board[sq + RU] == bpawn) return true;
        if(board[sq + LU] == bpawn) return true;
		/*knight*/
		current = plist[bknight];
		while(current) {
			if(sqatt[sq - current->sq].pieces & NM)
				return true;
			current = current->next;
		}
		/*bishop*/
		current = plist[bbishop];
		while(current) {
			if(sqatt[sq - current->sq].pieces & BM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*rook*/
		current = plist[brook];
		while(current) {
			if(sqatt[sq - current->sq].pieces & RM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*queen*/
		current = plist[bqueen];
		while(current) {
			if(sqatt[sq - current->sq].pieces & QM)
				if(blocked(current->sq,sq) == false)
					return true;
			current = current->next;
		}
		/*king*/
		if(sqatt[sq - plist[bking]->sq].pieces & KM)
			return true;
	}
	return false;
}

