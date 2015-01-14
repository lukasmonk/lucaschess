// Bitboard Perft by Jacob Hales (using Pradyumna Kannan's magic move generator)
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "magicmoves.h"
#include "engine.hpp"
#include "genmagic.hpp"
#include "hash.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "deprec.hpp"
#include "board.hpp"


// Define 64-bit integer
#ifndef __64_BIT_INTEGER_DEFINED__
   #define __64_BIT_INTEGER_DEFINED__
   #if defined(_MSC_VER) && _MSC_VER<1300
      typedef unsigned __int64 U64; //For the old microsoft compilers
   #else
      typedef unsigned long long  U64; //Supported by MSC 13.00+ and GCC
   #endif //defined(_MSC_VER) && _MSC_VER<1300
#endif //__64_BIT_INTEGER_DEFINED__


#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif


const char pc_char_map[12] = {
	'k', 'K', 'q', 'Q', 'r', 'R', 'b',
	'B', 'n', 'N', 'p', 'P'
};

// http://supertech.csail.mit.edu/papers/debruijn.pdf
//const U64 debruijn64 =	0x07EDD5E59A4E28C2ULL;

int index64[64] = {
	63,  0, 58,  1, 59, 47, 53,  2,
	60, 39, 48, 27, 54, 33, 42,  3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22,  4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16,  9, 12,
	44, 24, 15,  8, 23,  7,  6,  5
};

const unsigned char castle_spoiler[64] = {
	 13, 15, 15, 15, 15, 15, 15, 14,
	 15, 15, 15, 15, 15, 15, 15, 15,
	 15, 15, 15, 15, 15, 15, 15, 15,
	 15, 15, 15, 15, 15, 15, 15, 15,
	 15, 15, 15, 15, 15, 15, 15, 15,
	 15, 15, 15, 15, 15, 15, 15, 15,
	 15, 15, 15, 15, 15, 15, 15, 15,
	  7, 15, 15, 15, 15, 15, 15, 11
};

enum RANKS	{
	RANK_1,		RANK_2,		RANK_3,		RANK_4,
	RANK_5,		RANK_6,		RANK_7,		RANK_8
};

enum FILES	{
	FILE_A,		FILE_B,		FILE_C,		FILE_D,
	FILE_E,		FILE_F,		FILE_G,		FILE_H
};

//const U64 CLEAR_LEFT	= 0xFEFEFEFEFEFEFEFEULL;
//const U64 CLEAR_RIGHT	= 0x7F7F7F7F7F7F7F7FULL;

// Castling masks
const unsigned char castle_mask[2]		=	{ 12, 3 };
const unsigned char perm_mask_00[2]		=	{  4, 1 };
const unsigned char perm_mask_000[2]	=	{  8, 2 };

const U64 mask_00[2]	= { 0x6000000000000000ULL, 0x0000000000000060ULL };
const U64 mask_000[2]	= { 0x0E00000000000000ULL, 0x000000000000000EULL };

// Pawn move generation data
const int xoccu_wp[2]	= { X_OCCU, WP };
const int xoccu_bp[2]	= { X_OCCU, BP };
const int xoccu_tmp[2]	= { X_OCCU, TEMP };
const int woccu_wp[2]	= { W_OCCU, WP };
const int boccu_bp[2]	= { B_OCCU, BP };
const U64 mask_5_3[2]	= { 0x000000FF00000000ULL, 0x0000000000FF0000ULL};

const int p_forw[2]			= { -8, 8 };
const int inter_shift[2]	= {  8, 0 };
const int inter_shift2[2]	= { 16, 0 };

const U64 prom_mask[2]		= { 0x000000000000FF00ULL, 0xFF00000000000000ULL };
const U64 xprom_mask[2]		= { 0xFFFFFFFFFFFF00FFULL, 0x00FFFFFFFFFFFFFFULL };

const U64 ev_prom_mask[2]	= { 0x000000000000FF00ULL, 0x00FF000000000000ULL };
const U64 ev_xprom_mask[2]	= { 0xFFFFFFFFFFFF00FFULL, 0xFF00FFFFFFFFFFFFULL };





// Macros to simplify adding moves to a list (array of encoded unsigned integers)
// from {0-5}, to {6-11}, piece {12-15}, capt {16-19}, prom {20-21}
#define AddMove(from, to, pc) \
    list++->m = ((from) | ((to) << 6) | ((pc) << 12))

#define AddCapt(from, to, type, capt) \
    list++->m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16))

#define AddProm(from, to, type, capt) \
	list++->m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16) | (PROM_Q << 20))

#define AddUnderProm(from, to, type, capt) \
	list++->m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16) | (PROM_N << 20)); \
	list++->m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16) | (PROM_R << 20)); \
	list++->m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16) | (PROM_B << 20))

#define SET_ENPASSANT(hashk,sq) hashk ^= s_arghashkEnP[File(sq)]



// Engine data
//
U64 king_attacks[64];
U64 king_attacks_ring[64];
U64 knight_attacks[64];
U64 knight_future_attacks[64];
U64 pawn_attacks[2][64];
U64 squares_between[64][64];

U64 king_base[64];
U64 _IdxToU64[64];
static U64 _NotIdxToU64[64];
#define NotIdxToU64(idx)   (_NotIdxToU64[idx])

U64 nodes;

/*static int s_argcf[] = {
	~cfE1C1,~0,		~0,		~0,		~(cfE1G1 | cfE1C1), ~0,		~0,		~cfE1G1,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~cfE8C8,~0,		~0,		~0,		~(cfE8G8 | cfE8C8), ~0,		~0,		~cfE8G8,0,0,0,0,0,0,0,0,
};*/
static int s_argcf[64];


// Function declarations
//
//unsigned int PopLSB(U64 * val);
//unsigned int LSB(const U64 val);

void SetFromFEN(PCON pcon, PSTE pste, char* fen);
//void AddPiece(int idx, int piece);

U64 AttacksTo(const CON *pcon, int sqr);
static void Perft(PCON pcon, PSTE pste, int depth);

static void INLINE PinCheckUpdate(const CON *pcon, PSTE pste, int col);



/* de Bruijn */

const int       lsz64_tbl[64] =
{
    0, 31,  4, 33, 60, 15, 12, 34,
   61, 25, 51, 10, 56, 20, 22, 35,
   62, 30,  3, 54, 52, 24, 42, 19,
   57, 29,  2, 44, 47, 28,  1, 36,
   63, 32, 59,  5,  6, 50, 55,  7,
   16, 53, 13, 41,  8, 43, 46, 17,
   26, 58, 49, 14, 11, 40,  9, 45,
   21, 48, 39, 23, 18, 38, 37, 27,
};


/*popCount()
 *a noniterative population count of 1 bits in a quadword
 *
 *@param b - the quadword to be counted
 *@returns the number of 1 bits in b
 */
#define m1 0x5555555555555555ULL
#define m2 0x3333333333333333ULL
unsigned popCount1(bitboard b)
{
    unsigned        n;
    const bitboard  a = b - ((b >> 1) & m1);
    const bitboard  c = (a & m2) + ((a >> 2) & m2);
    n = (unsigned) ((c & 0xffffffff) + (c >> 32));
    n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F);
    n = (n & 0xFFFF) + (n >> 16);
    n = (n & 0xFF) + (n >> 8);
    return n;
}

/* BitBoard population count   */
/* (R. Scharnagl, 2005/Jul/20) */

#define msk1 0xEEEEEEEEUL
#define msk2 0xCCCCCCCCUL
#define msk3 0x88888888UL
#define msk4 0x0F0F0F0FUL

unsigned popCount2(const bitboard b)
{
  unsigned buf;
  unsigned acc;

  buf  = (unsigned)b;
  acc  = buf;
  acc -= ((buf &= msk1)>>1);
  acc -= ((buf &= msk2)>>2);
  acc -= ((buf &= msk3)>>3);
  buf  = (unsigned)(b>>32);
  acc += buf;
  acc -= ((buf &= msk1)>>1);
  acc -= ((buf &= msk2)>>2);
  acc -= ((buf &= msk3)>>3);
  acc = (acc & msk4)   + ((acc >> 4) & msk4);
  acc = (acc & 0xFFFF) + (acc >> 16);
  acc = (acc & 0xFF)   + (acc >> 8);
  return acc;
}

//#define USE_32_BIT_MULTIPLICATIONS

int XXX_transform(bitboard b, bitboard magic, int bits) {
#if defined(USE_32_BIT_MULTIPLICATIONS)
  return
    (unsigned)((int)b*(int)magic ^ (int)(b>>32)*(int)(magic>>32)) >> (32-bits);
#else
  return (int)((b * magic) >> (64 - bits));
#endif
}

unsigned horizontal_attack_index(bitboard occ) {
   unsigned u = (unsigned)occ | (unsigned)(occ >> 32) << 0;
   return u * 0x02020202 >> 26;
}

unsigned vertical_attack_index(bitboard occ) {
   unsigned u = (unsigned)occ | (unsigned)(occ >> 32) << 3;
   return u * 0x01041041 >> 26;
}

#define msigAddPiece(piece, bit ) {pcon->msigCount[ piece ] ++; bit = 1;}
//#define msigRemovePiece(piece, bit) {pcon->msigCount[ piece ] --; bit = pcon->msigCount[ piece ] != 0;}
#define msigRemovePiece(piece, bit) {if(!--pcon->msigCount[ piece ]) bit = 0;}

static void AddPiece(PCON pcon, PSTE pste, int idx, int piece)
{
	int col = piece & 1;
	if ((piece & 14) == KING) {
		pcon->king[col] = U8(idx);
	}

	pcon->pos[idx] = U8(piece);
	pcon->pieces[piece] |= IdxToU64(idx);
	pcon->pieces[B_OCCU|col] |= IdxToU64(idx);
	pcon->pieces[OCCU] = pcon->pieces[B_OCCU] | pcon->pieces[W_OCCU];
	pcon->pieces[X_OCCU] = ~pcon->pieces[OCCU];
    int valPc = 0;
    int valPn = 0;
    if( AnyColor(piece) == AnyColor(PAWN) )
        valPn = b_valPc[piece];
    else {
        valPc = b_valPc[piece];
    }
    if( piece & 1 ) {
	    pste->valPcUs += valPc;
	    pste->valPnUs += valPn;
    } else {
	    pste->valPcThem += valPc;
	    pste->valPnThem += valPn;
    }
}

void bGenMoves(PCON pcon, PSTE pste) {
#ifdef _DEBUG
// 8/8/8/1q1k1pp1/8/6P1/6P1/4R1K1 w - - 0 1
	if (Attacked(pcon, pcon->king[pste->side], pste->xside)) {
        Assert( pste->checkf );
    } else {
        Assert( pste->checkf == 0);
    }
#endif
//	if (Attacked(pcon, pcon->king[pste->side], pste->xside)) {
    if( pste->checkf ) {
		bGenEvasions(pcon, pste, pste->mlist_start);
	}
	else {
		CM * p_moves = bGenCaptures(pcon, pste, pste->mlist_start);
		bGenNonCaptures(pcon, pste, p_moves);
	}
}

static void Perft(PCON pcon, PSTE pste, int depth)
{
	if (depth == 0) { nodes++; return; }

    bGenMoves(pcon, pste);

    foreachmove(pste,pcm) {
		bMove(pcon, pste, pcm->m );
		Perft(pcon, pste + 1, depth - 1);
		bUndoMove(pcon, pste, pcm->m);
	}
}



void Print(const CON *pcon, const STE *pste)
{
	int i, j;

    printf("# +---------------+\n");

	for (i = 7; i >= 0; i--) {
        printf("# %d|", i + 1);

		for (j = 0; j < 8; j++) {

			if (!IsSet(Idx(i, j), pcon->pieces[OCCU]))
				printf(".");
			else
				printf("%c", pc_char_map[pcon->pos[Idx(i, j)]]);
			if (j < 7) printf(" ");
		}

		printf("|\n");
	}

    printf("# +---------------+\n");
    printf("#  a b c d e f g h\n");
    printf("# side=%d\n", pste->side );
    fflush(stdout);
}
/*
static void checkKing(const CON *pcon) {
    if( popCount(pcon->pieces[KING]) != 1 || popCount(pcon->pieces[KING|1]) != 1 ||
        (pcon->pieces[KING] & pcon->pieces[PAWN]) !=0) {
        Print(pcon, pcon->argste );
        Assert(0);
    }
}

static void checkPos(const CON *pcon, bool init) {
static U8 pos_save[64];
    if(init) {
        for(int i = 0; i < 64 ; i++ )
            pos_save[i] = pcon->pos[i];
    } else {
        for(int i = 0; i < 64 ; i++ )
            if( pos_save[i] != pcon->pos[i] ) {
                Assert(0);
            }

    }
}
*/
void InitBitboards(void)
{
	int i, j, k, file, rank;
	U64 temp;
    Assert(UINT_MAX >= 0xffffffff);

	initmagicmoves();

	// king/knight movement offsets
	int knight_off[] = { 15, 17, 10, 6, -6, -10, -17, -15 };
	int king_off[] = { 1, -1, 8, -8, 7, -7, 9, -9 };

	memset(squares_between, 0, sizeof(squares_between));

    for(i = 0; i < 64 ; i++) {
        // a table access is faster than a call to __allshl(1,idx)
        _IdxToU64[i] = (1ULL << (i));
        _NotIdxToU64[i] = ~_IdxToU64[i];
    }

	for (i = 0; i < 64; i++) {
		// Initialize king attacks
		for (j = 0; j < 8; j++) {
			int king_idx = i + king_off[j];
			int change = abs(File(i) - File(king_idx))
				+ abs(Rank(i) - Rank(king_idx));

			if (king_idx < 0 || king_idx > 63 || change > 2) {
				continue;
			}

			Set(king_idx, king_attacks[i]);
		}
        int kr = Rank(i);
        int kf = File(i);
        for(int r = -2; r <= 2; r++)
            for(int f = -2; f <= 2; f++) {
                int br = kr + r;
                int bf = kf + f;
                if( br >= 0 && br <= 7 && bf >= 0 && bf <= 7 )
                    Set( Idx(br,bf), king_base[i]);
            }

		// Initialize knight attacks
		for (j = 0; j < 8; j++) {
			int knight_idx = i + knight_off[j];
			int change = abs(File(i) - File(knight_idx))
				+ abs(Rank(i) - Rank(knight_idx));

			if (knight_idx < 0 || knight_idx > 63 || change != 3) {
				continue;
			}

			Set(knight_idx, knight_attacks[i]);
		}

		// Initialize squares_between
		for (j = 0; j < 64; j++) {
			int off = 0;
			int r_c = Rank(i) - Rank(j);
			int f_c = File(i) - File(j);

			if (j == i) continue;

			// Set an offset based on rank & file change
			if (f_c == 0) {
				off = (r_c < 0 ? 8 : -8);
			}
			else if (r_c == 0) {
				off = (f_c < 0 ? 1 : -1);
			}
			else if (abs(r_c) == abs(f_c)) {
				off =	(r_c < 0 ? 8 : -8);
				off +=	(f_c < 0 ? 1 : -1);
			}
			else continue;

			// Set bits along the offset
			for (k = 1; ((k * off) + i) != j; k++) {
				Set((k * off + i), squares_between[i][j]);
			}
		}

		// Initialize the pawn attacks
		file = File(i), rank = Rank(i);
		temp = 0;

		if ((file - 1) >= 0) Set((file - 1), temp);
		if ((file + 1) < 8) Set((file + 1), temp);

//		pawn_attacks[W][i] = temp << ((rank + 1) * 8);
//		pawn_attacks[B][i] = temp << ((rank - 1) * 8);
        if( rank < 7 )
		    pawn_attacks[W][i] = temp << ((rank + 1) * 8);
        else
            pawn_attacks[W][i] = 0;
        if( rank > 0 )
		    pawn_attacks[B][i] = temp << ((rank - 1) * 8);
        else
            pawn_attacks[B][i] = 0;

	}
    for(int sqr = 0; sqr < 64 ; sqr ++) {
        king_attacks_ring[sqr] = king_attacks[sqr] | IdxToU64(sqr);
        king_attacks_ring[sqr] |= soutOne(king_attacks_ring[sqr]);
        king_attacks_ring[sqr] |= nortOne(king_attacks_ring[sqr]);
        bitboard atk = knight_attacks[sqr];
        knight_future_attacks[sqr] = atk;
        while(atk) {
            int new_sqr = PopLSB(atk);
            knight_future_attacks[sqr] |= knight_attacks[new_sqr];
        }
    }
    // init castle flags, could be hard coded but it's needed for FRC
    // TODO : get position of king and rooks (should be called after setFen)

    for(int sqr = 0; sqr < 64; sqr++)
        s_argcf[sqr] = ~0;

    s_argcf[E1] &= ~(cfE1C1 | cfE1G1);
    s_argcf[H1] &= ~cfE1G1;
    s_argcf[A1] &= ~cfE1C1;

    s_argcf[E8] &= ~(cfE8G8 | cfE8C8);
    s_argcf[H8] &= ~cfE8G8;
    s_argcf[A8] &= ~cfE8C8;

}

static void my_swap(int &a, int &b) {
    int c = a;
    a = b;
    b = c;
}

void SetFromFEN(PCON pcon, PSTE pste, const char* fen)
{
	int idx = 0;

	// Reset the board

	pste->castle	= 0;
	pste->ep_t		= 0;
    pste->checkf    = 0;

	memset(pcon->pos, 0, sizeof(pcon->pos));
	memset(pcon->pieces, 0, sizeof(pcon->pieces));
	pste->hashkPc   = 0;    // Going to XOR the all pieces/pawns into this.
	pste->hashkPn   = 0;    // Going to XOR the pawns only into this.
	pste->valPcUs   = 0;
	pste->valPcThem = 0;
	pste->valPnUs   = 0;
	pste->valPnThem = 0;

    for(int i = 0; i < 2 ; i++)
        pcon->msigBit[i].msB = pcon->msigBit[i].msN = pcon->msigBit[i].msP = pcon->msigBit[i].msQ = pcon->msigBit[i].msR = 0;
    for(int i = 0; i < 16 ; i ++ )
        pcon->msigCount[i] = 0;

	// Parse board description
	for (int i = 7; i >=0; i--) {
		for (int j = 0; j <= 7; j++) {
			if (isdigit(fen[idx])) {
				j += atoi(&fen[idx]) - 1;
			}
			if (isalpha(fen[idx])) {
				int col = B;

				if (isupper(fen[idx])) {
					col = W;
				}
				switch (tolower(fen[idx])) {
					case 'p':
			            rehashBB( pste->hashkPc, BP|col, Idx(i, j));
			            rehashBB( pste->hashkPn, BP|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), PAWN|col);
                        msigAddPiece(PAWN|col, pcon->msigBit[col].msP);
						break;
					case 'n':
			            rehashBB( pste->hashkPc, BN|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), KNIGHT|col);
                        msigAddPiece(KNIGHT|col, pcon->msigBit[col].msN);
						break;
					case 'b':
			            rehashBB( pste->hashkPc, BB|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), BISHOP|col);
                        msigAddPiece(BISHOP|col, pcon->msigBit[col].msB);
						break;
					case 'r':
			            rehashBB( pste->hashkPc, BR|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), ROOK|col);
                        msigAddPiece(ROOK|col, pcon->msigBit[col].msR);
						break;
					case 'q':
			            rehashBB( pste->hashkPc, BQ|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), QUEEN|col);
                        msigAddPiece(QUEEN|col, pcon->msigBit[col].msQ);
						break;
					case 'k':
			            rehashBB( pste->hashkPc, BK|col, Idx(i, j));
						AddPiece(pcon, pste, Idx(i, j), KING|col);
						break;
				}

			}
			idx++;
		}
		idx++;
	}

	// Determine side to move
    if (fen[idx++] == 'w') {
		pste->side = W, pste->xside = B;
		pcon->gc.coStart = W;
    } else {
		pste->side = B, pste->xside = W;
		pcon->gc.coStart = B;
        pste->hashkPc = HashkSwitch(pste->hashkPc);
    }
    pste->xside = pste->side ^ 1;
    int side = pste->side;
    if( side == B ) {
        // need to swap values
	    my_swap(pste->valPcUs, pste->valPcThem);
	    my_swap(pste->valPnUs, pste->valPnThem);
    }

	pste->castle = cfNONE;
	// Parse castle field
	if (fen[++idx] == '-') {
		idx++;
	}
	else {
		while (isalpha(fen[idx])) {
			switch (fen[idx++]) {
				case 'K':
					pste->castle |= cfE1G1;
					break;
				case 'k':
					pste->castle |= cfE8G8;
					break;
				case 'Q':
					pste->castle |= cfE1C1;
					break;
				case 'q':
					pste->castle |= cfE8C8;
					break;
			}
		}
	}
    pste->hashkPc ^= s_arghashkCf[pste->castle];

	// Parse En Passant target
	if (fen[++idx] != '-') {
		int file = fen[idx] - 'a';
		int rank = fen[++idx] - '1';
		pste->ep_t = U8(Idx(rank, file));
        // verify if ep is real (wrong ep from Arena, then position is not found in book=
        if( pcon->pieces[PAWN|side] & pawn_attacks[side^1][pste->ep_t] ) {
            //rnbqkb1r/ppp1pppp/3p1n2/8/2PP4/5N2/PP2PPPP/RNBQKB1R b KQkq d3 0 3
            SET_ENPASSANT(pste->hashkPc, pste->ep_t);
        } else
            pste->ep_t = 0;
	}
	//
	//	Get fifty-move counter.
	//
    ++idx;
	pste->plyFifty = 0;
	for (;;) {
		if (!isdigit(fen[++idx]))
			break;
		pste->plyFifty *= 10;
		pste->plyFifty += fen[idx] - '0';
	}
//    ++idx;
	//
	//	Get start move counter.
	//
	pcon->gc.movStart = 0;
	for (;;) {
		if (!isdigit(fen[++idx]))
			break;
		pcon->gc.movStart *= 10;
		pcon->gc.movStart += fen[idx] - '0';
	}
	if (pcon->gc.movStart < 1)	// Start move must be 1 or more.
		pcon->gc.movStart = 1;

	// Initialize pin/check data
	PinCheckUpdate(pcon, pste, pste->side);

	// Contact checks
	if (pawn_attacks[pste->side][pcon->king[pste->side]] & pcon->pieces[PAWN|pste->xside]) {
		pste->checkf = 1;
	}
	if (knight_attacks[pcon->king[pste->side]] & pcon->pieces[KNIGHT|pste->xside]) {
		pste->checkf = 1;
	}
}

bool SetBoardFromFEN(PCON pcon, const char * szFen) {
	PSTE	pste = pcon->argste;
    SetFromFEN(pcon, pste, szFen);
	strcpy(pcon->gc.aszFen, szFen);	// Remember FEN.
	//
	//	Sets number of moves in this game (zero), and record the current
	//	position's hash key for rep checking.
	//
	pcon->gc.ccm = 0;				// Zero moves made so far.
	//
	//	"pcon->argste" contains some precomputed stuff, and this sets that
	//	up properly.
	//
	VFixSte(pcon);
    pste->mlist_start = pcon->move_list;
	//
	//	The position at the start of the game has occurred one time, so twice
	//	more is a draw.  This needs to be recorded in case it needs to be
	//	claimed.
	//
	pcon->gc.arghashk[0] = pste->hashkPc;
    return true;
}

// Based on Dr. Hyatt's algorithm (Crafty)
U64 AttacksTo(const CON *pcon, int sqr)
{
	return (king_attacks[sqr] & (pcon->pieces[BK] | pcon->pieces[WK]))
		  | (pawn_attacks[B][sqr] & pcon->pieces[WP])
		  | (pawn_attacks[W][sqr] & pcon->pieces[BP])
		  | (knight_attacks[sqr] & (pcon->pieces[WN] | pcon->pieces[BN]))
		  | (Bmagic(sqr, pcon->pieces[OCCU]) &
		  (pcon->pieces[WB] | pcon->pieces[BB] | pcon->pieces[WQ] | pcon->pieces[BQ]))
		  | (Rmagic(sqr, pcon->pieces[OCCU]) &
		  (pcon->pieces[WR] | pcon->pieces[BR] | pcon->pieces[WQ] | pcon->pieces[BQ]));
}



// Based on Dr. Hyatt's algorithm (Crafty)
int Attacked(const CON *pcon, int sqr, int col)
{
	if (Rmagic(sqr, pcon->pieces[OCCU]) &
		(pcon->pieces[ROOK|col] | pcon->pieces[QUEEN|col]))
		return 1;
	if (Bmagic(sqr, pcon->pieces[OCCU]) &
		(pcon->pieces[BISHOP|col] | pcon->pieces[QUEEN|col]))
		return 1;
	if (knight_attacks[sqr] & pcon->pieces[KNIGHT|col])
		return 1;
	if (king_attacks[sqr] & pcon->pieces[KING|col])
		return 1;
	if (pawn_attacks[col ^ 1][sqr] & pcon->pieces[PAWN|col])
		return 1;
	return 0;
}



// Generates a bitboard of pinned pieces for the side to move,
// and determines if there are any distance checks (sliders)
static void PinCheckUpdate(const CON *pcon, PSTE pste, int col)
{
	int xcol = col ^ 1;

	pste->pinner = 0;
	pste->pinned = 0;

	U64 sliders = (RAttackEmpty[pcon->king[col]] & (pcon->pieces[ROOK|xcol] | pcon->pieces[QUEEN|xcol]))
		| (BAttackEmpty[pcon->king[col]] & (pcon->pieces[BISHOP|xcol] | pcon->pieces[QUEEN|xcol]));
#ifdef _DEBUG
//	U64 sliders2 = (Rmagic(pcon->king[col], 0) & (pcon->pieces[ROOK|xcol] | pcon->pieces[QUEEN|xcol]))
//		| (Bmagic(pcon->king[col], 0) & (pcon->pieces[BISHOP|xcol] | pcon->pieces[QUEEN|xcol]));
//    Assert(sliders == sliders2);
#endif

	while (sliders) {
		int from = PopLSB(sliders);

		U64 pinned = squares_between[from][pcon->king[col]] & pcon->pieces[OCCU];

		// If it's neither a pin nor a check
		if (pinned & (pcon->pieces[W_OCCU - col] | (pinned - 1)))
			continue;

		if (pinned) {
			pste->pinned |= pinned;
            pste->pinner |= IdxToU64(from);
		}
		else
            pste->checkf += 2;
	}
}

static INLINE bitboard DiscoveredCheckers(const CON *pcon, const STE *pste, int col)
{
    col = col ^ 1;  // HACK
	int xcol = col ^ 1;

	bitboard checkers = 0;

	U64 sliders = (RAttackEmpty[pcon->king[col]] & (pcon->pieces[ROOK|xcol] | pcon->pieces[QUEEN|xcol]))
		| (BAttackEmpty[pcon->king[col]] & (pcon->pieces[BISHOP|xcol] | pcon->pieces[QUEEN|xcol]));

    // board   : --R--N--k  --R--N-pk   --R----pk
    // sliders : --R------  --R------   --R------
    // pinned  : -----N---  -----N-p-   -------p-
    // checkers: -----N---  ---------   ---------
    //              ok         two       not mine

	while (sliders) {
		int from = PopLSB(sliders);

		U64 pinned = squares_between[from][pcon->king[col]] & pcon->pieces[OCCU];

		// more stuff in between
		if (pinned & (pinned - 1))
			continue;
        // not my pieces
		if (pinned & pcon->pieces[B_OCCU | col])
			continue;

		if (pinned) {
			checkers |= pinned;
		}
	}
    return checkers;
}



CM* bGenCaptures(CON *pcon, STE *pste, CM * list)
{
    U64 pc, moves;
    const U8 * RESTRICT pos = pcon->pos;
    int side = pste->side;
    int xside = side ^ 1;
    const U8 * RESTRICT king = pcon->king;
//    checkKing(pcon);
//    checkPos(pcon, true);

    bitboard occu = pcon->pieces[OCCU];

	// Remove pinned pieces from their piece boards
	for (pc = pste->pinner; pc; ) {

		int to = PopLSB(pc);

		U64 pin_ray = squares_between[king[side]][to];
		int from = LSB(pcon->pieces[B_OCCU|side] & pin_ray);
		//Clear(from, pcon->pieces[pos[from]]);
        pcon->pieces[pos[from]] &= NotIdxToU64(from);

		switch (pos[from]) {
			case BQ:
				moves = Qmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, BQ, pos[to]);
				break;
			case WQ:
				moves = Qmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, WQ, pos[to]);
				break;
			case BR:
				moves = Rmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, BR, pos[to]);
				break;
			case WR:
				moves = Rmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, WR, pos[to]);
				break;
			case BB:
				moves = Bmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, BB, pos[to]);
				break;
			case WB:
				moves = Bmagic(from, occu) & IdxToU64(to);
				if (moves)	AddCapt(from, to, WB, pos[to]);
				break;
            case BP:
                if (from - 7 == to || from - 9 == to) {
                    if (Rank(to) > 0)
                        {AddCapt(from, to, BP, pos[to]);}
                    else
                        {AddProm(from, to, PROM|side, pos[to]);}
                }

				// Try the rare but possible pinnned e.p. capture
				if (pste->ep_t) {
					if (IdxToU64(pste->ep_t) & pin_ray) {
						if (from - 7 == pste->ep_t
							|| from - 9 == pste->ep_t) {
							AddCapt(from, pste->ep_t, BP, B_EP);
						}
					}
				}
				break;
            case WP:
                if (from + 7 == to || from + 9 == to) {
                    if (Rank(to) < 7)
                        {AddCapt(from, to, WP, pos[to]);}
                    else
                        {AddProm(from, to, PROM|side, pos[to]);}
                }

				// Try the rare but possible pinnned e.p. capture
				if (pste->ep_t) {
					if (IdxToU64(pste->ep_t) & pin_ray) {
						if (from + 7 == pste->ep_t
							|| from + 9 == pste->ep_t) {
							AddCapt(from, pste->ep_t, WP, W_EP);
						}
					}
				}
				break;
		}
	}

    bitboard capturable = pcon->pieces[B_OCCU|xside];

	// - Knights -
	for (pc = pcon->pieces[KNIGHT|side]; pc; ) {
		int from = PopLSB(pc);
		moves = knight_attacks[from] & capturable;

		while (moves) {
			int to = PopLSB(moves);
			AddCapt(from, to, KNIGHT|side, pos[to]);
		}
	}

	// - Bishop -
	for (pc = pcon->pieces[BISHOP|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Bmagic(from, occu) & capturable;

		while (moves) {
			int to = PopLSB(moves);
			AddCapt(from, to, BISHOP|side, pos[to]);
		}
	}

	// - Rook -
	for (pc = pcon->pieces[ROOK|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Rmagic(from, occu) & capturable;

		while (moves) {
			int to = PopLSB(moves);
			AddCapt(from, to, ROOK|side, pos[to]);
		}
	}

	// - Queen -
	for (pc = pcon->pieces[QUEEN|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Qmagic(from, occu) & capturable;
//        Assert((moves & (pcon->pieces[KING|W]|pcon->pieces[KING|B])) == 0);
		while (moves) {
			int to = PopLSB(moves);
			AddCapt(from, to, QUEEN|side, pos[to]);
		}
	}

	// - King -
	moves = king_attacks[king[side]] & capturable;
	while (moves) {
		int to = PopLSB(moves);
		if (Attacked(pcon, to, xside))
			continue;
		AddCapt(king[side], to, KING|side, pos[to]);
	}

    int sqr[2];

	// - Pawns forward one -
	pcon->pieces[TEMP] = (pcon->pieces[xoccu_wp[side]] << 8)
		& pcon->pieces[xoccu_bp[xside]];

	moves = pcon->pieces[TEMP] & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 8;
		AddProm(sqr[0], sqr[1], PROM|side, 0);
	}

	// - Pawns seven-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_LEFT) << 7)
		& pcon->pieces[boccu_bp[xside]];

	moves = pc & xprom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 7;
		AddCapt(sqr[0], sqr[1], PAWN|side, pos[sqr[1]]);
	}

	moves = pc & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 7;
		AddProm(sqr[0], sqr[1], PROM|side, pos[sqr[1]]);
	}

	// - Pawns nine-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_RIGHT) << 9)
		& pcon->pieces[boccu_bp[xside]];

	moves = pc & xprom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 9;
		AddCapt(sqr[0], sqr[1], PAWN|side, pos[sqr[1]]);
	}

	moves = pc & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 9;
		AddProm(sqr[0], sqr[1], PROM|side, pos[sqr[1]]);
	}

	// - En Passant -
	if (pste->ep_t) {
		pc = (pcon->pieces[PAWN|side] & ~pste->pinned)
			& pawn_attacks[xside][pste->ep_t];

		while (pc) {
			U64 move_ep;
			int from = PopLSB(pc);

			move_ep = IdxToU64(from) | IdxToU64(pste->ep_t)
				| IdxToU64(pste->ep_t - p_forw[side]);

			occu ^= move_ep;

			if (!(Rmagic(king[side], occu) & (pcon->pieces[QUEEN|xside] | pcon->pieces[ROOK|xside]))) {
				AddCapt(from, pste->ep_t, PAWN|side, EP_CAPT|side);
			}

			occu ^= move_ep;
		}
	}
//    checkKing(pcon);

	// Add pinned pieces back to their piece boards
	for (pc = pste->pinned; pc; ) {
		int to = PopLSB(pc);
		//Set(to, pcon->pieces[pos[to]]);
        pcon->pieces[pos[to]] |= IdxToU64(to);
	}
//    checkKing(pcon);

//    checkPos(pcon, false);
    (pste+1)->mlist_start = list;
	return list;
}



CM* bGenNonCaptures(PCON pcon, PSTE pste, CM * list)
{
	U64 pc, moves;
    const U8 * RESTRICT pos = pcon->pos;
    int side = pste->side;
    int xside = pste->xside;
    const U8 * RESTRICT king = pcon->king;
//    checkPos(pcon, true);
    bitboard occu = pcon->pieces[OCCU];
    bitboard x_occu = ~occu;

	// Remove pinned pieces from their piece boards
	for (pc = pste->pinner; pc; ) {

		U64 pin_ray;
		int to = PopLSB(pc);

		pin_ray = squares_between[king[side]][to];
		int from = LSB(pcon->pieces[B_OCCU|side] & pin_ray);

//		Clear(from, pcon->pieces[pos[from]]);
        pcon->pieces[pos[from]] &= NotIdxToU64(from);

		switch (pos[from]) {
			case BQ:
				moves = Qmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), BQ);
				break;
			case WQ:
				moves = Qmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), WQ);
				break;
			case BR:
				moves = Rmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), BR);
				break;
			case WR:
				moves = Rmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), WR);
				break;
			case BB:
				moves = Bmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), BB);
				break;
			case WB:
				moves = Bmagic(from, occu) & pin_ray;
				while (moves) AddMove(from, PopLSB(moves), WB);
				break;
			case BP:
				if (Rank(from) == RANK_2) {
					if (from - 7 == to || from - 9 == to) {
						AddUnderProm(from, to, PROM|side, pos[to]);
					}
				}

				// Straight moves
				if ((IdxToU64(from) >> 8) & pin_ray) {
//					if (Rank(from) == RANK_7 && pos[from - 16] == 0) {
					if (Rank(from) == RANK_7 && pos[from - 16] == 0
                        && (from - 16) != king[B]) {
						AddMove(from, from - 16, BP);
					}
					AddMove(from, from - 8, BP);
				}
				break;
			case WP:
				if (Rank(from) == RANK_7) {
					if (from + 7 == to || from + 9 == to) {
						AddUnderProm(from, to, PROM|side, pos[to]);
					}
				}

				// Straight moves
				if ((IdxToU64(from) << 8) & pin_ray) {
//					if (Rank(from) == RANK_2 && pos[sqr[0] + 16] == 0) {
					if (Rank(from) == RANK_2 && pos[from + 16] == 0
                        && (from + 16) != king[B]) {
						AddMove(from, from + 16, WP);
					}
					AddMove(from, from + 8, WP);
				}
				break;
		}
	}

	// - Knights -
	for (pc = pcon->pieces[KNIGHT|side]; pc; ) {
		int from = PopLSB(pc);
		moves = knight_attacks[from] & x_occu;

		while (moves) {
			AddMove(from, PopLSB(moves), KNIGHT|side);
		}
	}

	// - Bishop -
	for (pc = pcon->pieces[BISHOP|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Bmagic(from, occu) & x_occu;

		while (moves) {
			AddMove(from, PopLSB(moves), BISHOP|side);
		}
	}

	// - Rook -
	for (pc = pcon->pieces[ROOK|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Rmagic(from, occu) & x_occu;

		while (moves) {
			AddMove(from, PopLSB(moves), ROOK|side);
		}
	}

	// - Queen -
	for (pc = pcon->pieces[QUEEN|side]; pc; ) {
		int from = PopLSB(pc);
		moves = Qmagic(from, occu) & x_occu;

		while (moves) {
			AddMove(from, PopLSB(moves), QUEEN|side);
		}
	}

	// - King -
	moves = king_attacks[king[side]] & x_occu;

	while (moves) {
		int to = PopLSB(moves);
		if (Attacked(pcon, to, xside))
			continue;
		AddMove(king[side], to, KING|side);
	}

	// - Castling -
	if (pste->castle & castle_mask[side]) {
		if (pste->castle & perm_mask_00[side]
			&& (occu & mask_00[side]) == 0
			&& !Attacked(pcon, king[side] + 1, xside)
			&& !Attacked(pcon, king[side] + 2, xside)){
				AddMove(king[side], king[side] + 2, CASTLE|side);
		}
		if (pste->castle & perm_mask_000[side]
			&& (occu & mask_000[side]) == 0
			&& !Attacked(pcon, king[side] - 1, xside)
			&& !Attacked(pcon, king[side] - 2, xside)){
				AddMove(king[side], king[side] - 2, CASTLE|side);
		}
	}

	int sqr[2];
	// - Pawns forward one -
	pcon->pieces[TEMP] = (pcon->pieces[xoccu_wp[side]] << 8)
		& pcon->pieces[xoccu_bp[xside]];

	moves = pcon->pieces[TEMP] & xprom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 8;
		AddMove(sqr[0], sqr[1], pos[sqr[0]]);
	}

	moves = pcon->pieces[TEMP] & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 8;
		AddUnderProm(sqr[0], sqr[1], PROM|side, 0);
	}

	// - Pawns forward two -
	moves = ((pcon->pieces[xoccu_tmp[side]] & mask_5_3[side]) << (8 << xside))
		& pcon->pieces[xoccu_tmp[xside]];

	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 16;
		AddMove(sqr[0], sqr[1], pos[sqr[0]]);
	}

	// - Pawns seven-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_LEFT) << 7)
		& pcon->pieces[boccu_bp[xside]];

	moves = pc & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 7;
		AddUnderProm(sqr[0], sqr[1], PROM|side, pos[sqr[1]]);
	}

	// - Pawns nine-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_RIGHT) << 9)
		& pcon->pieces[boccu_bp[xside]];

	moves = pc & prom_mask[side];
	while (moves) {
		sqr[side] = PopLSB(moves);
		sqr[xside] = sqr[side] - 9;
		AddUnderProm(sqr[0], sqr[1], PROM|side, pos[sqr[1]]);
	}

	// Add pinned pieces back to their piece boards
	for (pc = pste->pinned; pc; ) {
		int to = PopLSB(pc);
		//Set(to, pcon->pieces[pos[to]]);
        pcon->pieces[pos[to]] |= IdxToU64(to);
	}
//    checkPos(pcon, false);

    (pste+1)->mlist_start = list;
	return list;
}


CM* bGenPasserPush(PCON pcon, PSTE pste, CM * list)
{
	U64 pc, moves;
    const U8 *pos = pcon->pos;
    int side = pste->side;
    int xside = pste->xside;
    const unsigned char *king = pcon->king;

	// Remove pinned pieces from their piece boards
	for (pc = pste->pinner; pc; ) {

		int to = PopLSB(pc);
		U64 pin_ray = squares_between[king[side]][to];
		int from = LSB(pcon->pieces[B_OCCU|side] & pin_ray);

        pcon->pieces[pos[from]] &= NotIdxToU64(from);

		switch (pos[from]) {
			case BP:
				// Straight moves
				if ((IdxToU64(from) >> 8) & pin_ray) {
                    if(Rank(from) == 2)
					    AddMove(from, from - 8, BP);
				}
				break;
			case WP:
				// Straight moves
				if ((IdxToU64(from) << 8) & pin_ray) {
                    if(Rank(from) == 5)
					    AddMove(from, from + 8, WP);
				}
				break;
		}
	}
    if(side == W ) {
        moves = nortOne(pcon->pieces[PAWN|W]) & ~pcon->pieces[OCCU] & rank7Mask;
        while (moves) {
            int to = PopLSB(moves);
            int from = to - 8;
            AddMove(from, to, WP);
        }
    } else {
        moves = soutOne(pcon->pieces[PAWN|B]) & ~pcon->pieces[OCCU] & rank2Mask;
        while (moves) {
            int to = PopLSB(moves);
            int from = to + 8;
            AddMove(from, to, BP);
        }
    }

	// Add pinned pieces back to their piece boards
	for (pc = pste->pinned; pc; ) {
		int to = PopLSB(pc);
        pcon->pieces[pos[to]] |= IdxToU64(to);
	}

    (pste+1)->mlist_start = list;
	return list;
}

CM* bGenPawnPush(PCON pcon, PSTE pste, CM * list)
{
	U64 pc, moves;
    const U8 *pos = pcon->pos;
    int side = pste->side;
    int xside = pste->xside;
    const unsigned char *king = pcon->king;

	// Remove pinned pieces from their piece boards
	for (pc = pste->pinner; pc; ) {

		int to = PopLSB(pc);
		U64 pin_ray = squares_between[king[side]][to];
		int from = LSB(pcon->pieces[B_OCCU|side] & pin_ray);

        pcon->pieces[pos[from]] &= NotIdxToU64(from);

		switch (pos[from]) {
			case BP:
				// Straight moves
				if ((IdxToU64(from) >> 8) & pin_ray) {
                    if(Rank(from) > 1)
					    AddMove(from, from - 8, BP);
				}
				break;
			case WP:
				// Straight moves
				if ((IdxToU64(from) << 8) & pin_ray) {
                    if(Rank(from) < 6)
					    AddMove(from, from + 8, WP);
				}
				break;
		}
	}
    if(side == W ) {
        moves = nortOne(pcon->pieces[PAWN|W]) & ~pcon->pieces[OCCU] & !rank8Mask;
        while (moves) {
            int to = PopLSB(moves);
            int from = to - 8;
            AddMove(from, to, WP);
        }
    } else {
        moves = soutOne(pcon->pieces[PAWN|B]) & ~pcon->pieces[OCCU] & !rank1Mask;
        while (moves) {
            int to = PopLSB(moves);
            int from = to + 8;
            AddMove(from, to, BP);
        }
    }

	// Add pinned pieces back to their piece boards
	for (pc = pste->pinned; pc; ) {
		int to = PopLSB(pc);
        pcon->pieces[pos[to]] |= IdxToU64(to);
	}

    (pste+1)->mlist_start = list;
	return list;
}

CM* bGenPseudoCheck(PCON pcon, PSTE pste, CM * list)
{
	U64 pc, moves;
    const U8 * RESTRICT pos = pcon->pos;
    const int side = pste->side;
    const int xside = pste->xside;
    const U8 * RESTRICT king = pcon->king;
    const U8 kingSquare = king[xside];

    const bitboard discoverSquares = DiscoveredCheckers(pcon, pste, side);
    const bitboard not_discoverSquares = ~discoverSquares;

    const bitboard occu = pcon->pieces[OCCU];
    const bitboard x_occu = ~occu;
    const bitboard bishopCheckSquares = Bmagic(kingSquare, occu) & x_occu;
    const bitboard rookCheckSquares = Rmagic(kingSquare, occu) & x_occu;
    const bitboard queenCheckSquares = bishopCheckSquares | rookCheckSquares;
    const bitboard knightCheckSquares = knight_attacks[kingSquare] & x_occu;

	// Remove pinned pieces from their piece boards
	for (pc = pste->pinner; pc; ) {

		int to = PopLSB(pc);

		U64 pin_ray = squares_between[king[side]][to];
		int from = LSB(pcon->pieces[B_OCCU|side] & pin_ray);

        pcon->pieces[pos[from]] &= NotIdxToU64(from);
#if 1
		switch (pos[from]) {
			case BQ:
				moves = Qmagic(from, occu) & pin_ray & queenCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), BQ);
				break;
			case WQ:
				moves = Qmagic(from, occu) & pin_ray & queenCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), WQ);
				break;
			case BR:
				moves = Rmagic(from, occu) & pin_ray & rookCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), BR);
				break;
			case WR:
				moves = Rmagic(from, occu) & pin_ray & rookCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), WR);
				break;
			case BB:
				moves = Bmagic(from, occu) & pin_ray & bishopCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), BB);
				break;
			case WB:
				moves = Bmagic(from, occu) & pin_ray & bishopCheckSquares;
				while (moves) AddMove(from, PopLSB(moves), WB);
				break;
			case BP:
				// Straight moves
				if ((IdxToU64(from) >> 8) & pin_ray) {
					if (Rank(from) == RANK_7 && pos[from - 16] == 0
                        && (from - 16) != king[B]) {
						AddMove(from, from - 16, BP);
					}
					AddMove(from, from - 8, BP);
				}
				break;
			case WP:
				// Straight moves
				if ((IdxToU64(from) << 8) & pin_ray) {
					if (Rank(from) == RANK_2 && pos[from + 16] == 0
                        && (from + 16) != king[B]) {
						AddMove(from, from + 16, WP);
					}
					AddMove(from, from + 8, WP);
				}
				break;
		}
#endif
	}

    // - King -
    if( IdxToU64( king[side] ) & discoverSquares ) {
        // same problem as pawns, exclude moves in (opp-)direction of king
	    moves = king_attacks[king[side]] & x_occu &~ (RAttackEmpty[kingSquare] | BAttackEmpty[kingSquare]);

	    while (moves) {
		    int to = PopLSB(moves);
		    if (Attacked(pcon, to, xside))
			    continue;
		    AddMove(king[side], to, KING|side);
	    }
    }
#if 1

	// - Castling -
	if (pste->castle & castle_mask[side]) {
		if (pste->castle & perm_mask_00[side]
            && (File(kingSquare) == filF)
			&& (occu & mask_00[side]) == 0
			&& !Attacked(pcon, king[side] + 1, xside)
			&& !Attacked(pcon, king[side] + 2, xside)){
				AddMove(king[side], king[side] + 2, CASTLE|side);
		}
		if (pste->castle & perm_mask_000[side]
            && (File(kingSquare) == filD)
			&& (occu & mask_000[side]) == 0
			&& !Attacked(pcon, king[side] - 1, xside)
			&& !Attacked(pcon, king[side] - 2, xside)){
				AddMove(king[side], king[side] - 2, CASTLE|side);
		}
	}

#endif

	// - Knights -
	for (pc = pcon->pieces[KNIGHT|side] & discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = knight_attacks[from] & x_occu;
		while (moves) {
			AddMove(from, PopLSB(moves), KNIGHT|side);
		}
	}
	for (pc = pcon->pieces[KNIGHT|side] & not_discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = knight_attacks[from] & knightCheckSquares;
		while (moves) {
			AddMove(from, PopLSB(moves), KNIGHT|side);
		}
	}

	// - Bishop -
	for (pc = pcon->pieces[BISHOP|side] & discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = Bmagic(from, occu) & x_occu;
		while (moves) {
			AddMove(from, PopLSB(moves), BISHOP|side);
		}
	}
	for (pc = pcon->pieces[BISHOP|side] & not_discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = Bmagic(from, occu) & bishopCheckSquares;
		while (moves) {
			AddMove(from, PopLSB(moves), BISHOP|side);
		}
	}

	// - Rook -
	for (pc = pcon->pieces[ROOK|side] & discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = Rmagic(from, occu) & x_occu;
		while (moves) {
			AddMove(from, PopLSB(moves), ROOK|side);
		}
	}
	for (pc = pcon->pieces[ROOK|side] & not_discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = Rmagic(from, occu) & rookCheckSquares;
		while (moves) {
			AddMove(from, PopLSB(moves), ROOK|side);
		}
	}

	// - Queen -
/*	for (pc = pcon->pieces[QUEEN|side] & discoverSquares; pc; ) {
		int from = PopLSB(pc);
		moves = Qmagic(from, occu) & x_occu;
		while (moves) {
			AddMove(from, PopLSB(moves), QUEEN|side);
		}
	}*/
	for (pc = pcon->pieces[QUEEN|side] /*& not_discoverSquares*/; pc; ) {
		int from = PopLSB(pc);
		moves = Qmagic(from, occu) & queenCheckSquares;
		while (moves) {
			AddMove(from, PopLSB(moves), QUEEN|side);
		}
	}

    if(side == W ) {
    	// - Pawns forward one -
        moves = nortOne(pcon->pieces[PAWN|W] & discoverSquares) & x_occu & ~Rank8Mask;
        moves &= ~pawnFileMask[File(kingSquare)];
        while (moves) {
            int to = PopLSB(moves);
            int from = to - 8;
            AddMove(from, to, WP);
        }
	    moves = soutOne(pawn_attacks[xside][kingSquare] & x_occu) & pcon->pieces[PAWN|W] & not_discoverSquares;
        while (moves) {
            int from = PopLSB(moves);
            int to = from + 8;
            AddMove(from, to, WP);
        }
	    // - Pawns forward two -
        moves = nortOne(nortOne(pcon->pieces[PAWN|W] & Rank2Mask & discoverSquares) & x_occu) & x_occu;
        moves &= ~pawnFileMask[File(kingSquare)];
        while (moves) {
            int to = PopLSB(moves);
            int from = to - 16;
            AddMove(from, to, WP);
        }
	    moves = soutOne(soutOne(pawn_attacks[xside][kingSquare] & x_occu) & x_occu) & pcon->pieces[PAWN|W] & Rank2Mask & not_discoverSquares;
        while (moves) {
            int from = PopLSB(moves);
            int to = from + 16;
            AddMove(from, to, WP);
        }
    } else {
        moves = soutOne(pcon->pieces[PAWN|B] & discoverSquares) & x_occu & ~Rank1Mask;
        moves &= ~pawnFileMask[File(kingSquare)];
        while (moves) {
            int to = PopLSB(moves);
            int from = to + 8;
            AddMove(from, to, BP);
        }
	    moves = nortOne(pawn_attacks[xside][kingSquare] & x_occu) & pcon->pieces[PAWN|B] & not_discoverSquares;
        while (moves) {
            int from = PopLSB(moves);
            int to = from - 8;
            AddMove(from, to, BP);
        }

        moves = soutOne(soutOne(pcon->pieces[PAWN|B] & Rank7Mask & discoverSquares) & x_occu) & x_occu;
        moves &= ~pawnFileMask[File(kingSquare)];
        while (moves) {
            int to = PopLSB(moves);
            int from = to + 16;
            AddMove(from, to, BP);
        }
	    moves = nortOne(nortOne(pawn_attacks[xside][kingSquare] & x_occu) & x_occu) & pcon->pieces[PAWN|B] & Rank7Mask & not_discoverSquares;
        while (moves) {
            int from = PopLSB(moves);
            int to = from - 16;
            AddMove(from, to, BP);
        }
    }

    // under prom removed

    // under prom removed

	// Add pinned pieces back to their piece boards
	for (pc = pste->pinned; pc; ) {
		int to = PopLSB(pc);
        pcon->pieces[pos[to]] |= IdxToU64(to);
	}

    (pste+1)->mlist_start = list;
	return list;
}


// Generate moves that move out of check. Check flags:
// [1: 0001] Contact check		[3: 0011] Contact/Distance check
// [2: 0010] Distance check		[4: 0100] Distance double-check
CM* bGenEvasions(PCON pcon, PSTE pste, CM * list)
{
	U64 pc, moves, x_pin = ~pste->pinned;
    const U8 * RESTRICT pos = pcon->pos;
    int side = pste->side;
    int xside = side ^ 1;
    const U8 * RESTRICT king = pcon->king;
    if( list == NULL )
        list = pste->mlist_start;

//    checkPos(pcon, true);
//    checkKing(pcon);
    pc = king_attacks[king[side]];

	// Prevent the king from hiding behind himself
	//Clear(king[side], pcon->pieces[OCCU]);
    pcon->pieces[OCCU] &= NotIdxToU64(king[side]);

	// - King Captures -
	for (moves = pc & pcon->pieces[B_OCCU|xside]; moves; ) {
		int to = PopLSB(moves);
		if (Attacked(pcon, to, xside))
			continue;
		AddCapt(king[side], to, KING|side, pos[to]);
	}

	// - King Moves -
	for (moves = pc & pcon->pieces[X_OCCU]; moves; ) {
		int to = PopLSB(moves);
		if (Attacked(pcon, to, xside))
			continue;
		AddMove(king[side], to, KING|side);
	}

	// Add the king back onto the occupancy
	//Set(king[side], pcon->pieces[OCCU]);
    pcon->pieces[OCCU] |= IdxToU64(king[side]);

	// If it's not double check, other moves are possible
	if (pste->checkf <= 2) {
		bitboard interpose = 0;
        bitboard targ = AttacksTo(pcon, king[side]) & pcon->pieces[B_OCCU|xside];
        int sqr[2];
        int to = sqr[1] = LSB(targ);

		// Set the interpose board
		if (pste->checkf == 2) {
			interpose = squares_between[king[side]][to];
		}

		// - Knights -
		for (pc = pcon->pieces[KNIGHT|side] & x_pin; pc; ) {
			int from = PopLSB(pc);
			moves = knight_attacks[from];

			if (moves & targ) AddCapt(from, to, KNIGHT|side, pos[to]);
			moves &= interpose;
			while (moves) AddMove(from, PopLSB(moves), KNIGHT|side);
		}

		// - Bishops -
		for (pc = pcon->pieces[BISHOP|side] & x_pin; pc; ) {
			int from = PopLSB(pc);
			moves = Bmagic(from, pcon->pieces[OCCU]);

			if (moves & targ) AddCapt(from, to, BISHOP|side, pos[to]);
			moves &= interpose;
			while (moves) AddMove(from, PopLSB(moves), BISHOP|side);
		}

		// - Rooks -
		for (pc = pcon->pieces[ROOK|side] & x_pin; pc; ) {
			int from = PopLSB(pc);
			moves = Rmagic(from, pcon->pieces[OCCU]);

			if (moves & targ) AddCapt(from, to, ROOK|side, pos[to]);
			moves &= interpose;
			while (moves) AddMove(from, PopLSB(moves), ROOK|side);
		}

		// - Queens -
		for (pc = pcon->pieces[QUEEN|side] & x_pin; pc; ) {
			int from = PopLSB(pc);
			moves = Qmagic(from, pcon->pieces[OCCU]);

			if (moves & targ) AddCapt(from, to, QUEEN|side, pos[to]);
			moves &= interpose;
			while (moves) AddMove(from, PopLSB(moves), QUEEN|side);
		}

		// - Pawn Captures -
		pc = pawn_attacks[xside][to] & (pcon->pieces[PAWN|side] & x_pin);

		moves = pc & ev_xprom_mask[side];
#if 0
        printf("# move pawn1 xside=%d sqr1=%d xpin=%I64x atk=%I64x pwn=%I64x\n", xside, to, x_pin, pawn_attacks[xside][to],
            pcon->pieces[PAWN|side]);
        VDumpPawnMaps("111",
            &pcon->pieces[PAWN|side], "pwn",
            &pawn_attacks[xside][to], "atk",
            &pc, "atd");
#endif
		while (moves) {
			int from = PopLSB(moves);
			AddCapt(from, to, PAWN|side, pos[to]);
		}

		moves = pc & ev_prom_mask[side];
		while (moves) {
			int from = PopLSB(moves);
			AddProm(from, to, PROM|side, pos[to]);
			AddUnderProm(from, to, PROM|side, pos[to]);
		}

		// - En Passant Evasion -
		if (pste->ep_t) {
			// Try to move attacker to e.p. square
		    // !! After e.p. evasions, attacker (sqr[1]) has been lost
			sqr[1] += p_forw[side];

			if (sqr[1] == pste->ep_t) {
				moves = (pawn_attacks[xside][sqr[1]] & pcon->pieces[PAWN|side]) & x_pin;
				while (moves) {
					AddCapt(PopLSB(moves), sqr[1], PAWN|side, EP_CAPT|side);
				}
			}
		}

		// After e.p. evasions, attacker (sqr[1]) has been lost

		// - Pawns forward one -
		pc = pcon->pieces[PAWN|side]; // Back-up pawn bitboard
		pcon->pieces[PAWN|side] &= x_pin;

		pcon->pieces[TEMP] = (pcon->pieces[xoccu_wp[side]] << 8)
			& pcon->pieces[xoccu_bp[xside]];

		moves = pcon->pieces[TEMP] & xprom_mask[side] & (interpose << inter_shift[side]);
		while (moves) {
			sqr[side] = PopLSB(moves);
			sqr[xside] = sqr[side] - 8;
			AddMove(sqr[0], sqr[1], pos[sqr[0]]);
		}

		moves = pcon->pieces[TEMP] & prom_mask[side] & (interpose << inter_shift[side]);
		while (moves) {
			sqr[side] = PopLSB(moves);
			sqr[xside] = sqr[side] - 8;
			AddProm(sqr[0], sqr[1], PROM|side, pos[sqr[1]]);
			AddUnderProm(sqr[0], sqr[1], PROM|side, 0);
		}

		// - Pawns forward two -
		moves = ((pcon->pieces[xoccu_tmp[side]] & mask_5_3[side]) << (8 << xside))
			& pcon->pieces[xoccu_tmp[xside]];

		moves &= (interpose << inter_shift2[side]);
		while (moves) {
			sqr[side] = PopLSB(moves);
			sqr[xside] = sqr[side] - 16;
			AddMove(sqr[0], sqr[1], pos[sqr[0]]);
		}

		pcon->pieces[PAWN|side] = pc; // Restore pawn bitboard
	}
//    checkKing(pcon);
//    checkPos(pcon, false);

    (pste+1)->mlist_start = list;
	return list;
}

void checkBoard(PCON pcon, PSTE pste) {
    unsigned int c_mats = 0;
    for(int i = KING; i <= PAWN+1; i++) {
        U64 mats = pcon->pieces[i];
        while(mats) {
            c_mats++;
            int p = PopLSB(mats);
            if( pcon->pos[p] != i) {
                printf("%d\n", p);
                Assert(false);
            }
        }
    }
    Assert(c_mats == popCount(pcon->pieces[OCCU]));
}

void bMove(PCON pcon, PSTE pste, U32 move)
{
    U8 * RESTRICT pos = pcon->pos;
    U8 * RESTRICT king = pcon->king;
    STE * RESTRICT npste = pste + 1;  // next stack element

    pste->lastMove = move;  // to display the current variation
#ifdef _DEBUG
    checkBoard(pcon,pste);
#endif

    // Update the current status and ply
    U8 old_castle   = pste->castle;
//    npste->castle   = old_castle;
    npste->ep_t     = 0;
    npste->checkf   = 0;
    npste->evaluated= false;
//    npste->danger   = false;
    npste->side     = pste->side ^ 1;
    npste->xside    = pste->side;
    npste->fNull    = false;        // By default, null-move is allowed.
    //
    //	Initialize various values for the next ply.
    //
    npste->plyFifty = pste->plyFifty + 1;
    npste->hashkPc  = HashkSwitch(pste->hashkPc);
    if( pste->ep_t )
        SET_ENPASSANT(npste->hashkPc, pste->ep_t);
    npste->hashkPn  = pste->hashkPn;
    npste->valPcUs  = pste->valPcThem;
    npste->valPcThem= pste->valPcUs;
    npste->valPnUs  = pste->valPnThem;
    npste->valPnThem= pste->valPnUs;

//    pste++;
//    pste = 0;

    // Decompress the move
    int from            = From(move);
    int to              = To(move);
    bitboard to_64      = IdxToU64(to);
    bitboard from_64    = IdxToU64(from);
    bitboard move_64    = to_64 | from_64;

    U8 new_castle       = old_castle & s_argcf[from] & s_argcf[to];
    npste->castle       = new_castle;
    npste->hashkPc      ^= s_arghashkCf[old_castle] ^ s_arghashkCf[new_castle];

    int piece = Pc(move);
    // Perform captures, if any
    int captured = Capt(move);
    // Move the piece on the array board
#ifdef _DEBUG
    // capturing own material
    if(pos[to] && ((pos[to] & 1) == (pos[from] & 1)) )
        Assert(false);
#endif
    pos[to]		= pos[from];
    pos[from]	= 0;

//    npste->prevCaptValue    = 0;
    npste->prevCaptSqr      = -1;   // HACK

    // note : king can not be captured
    if(captured) {
        npste->plyFifty = 0;                        // Capture resets this.
//        npste->prevCaptValue = 1;//b_valPc[ captured ]; // the eral value is not used
        npste->prevCaptSqr   = to;

        switch (captured) {
            case BQ:
                pcon->pieces[B_OCCU]	^= to_64;
                pcon->pieces[BQ]		^= to_64;
                npste->valPcUs          -= valQUEEN;
                // XOR the piece out of the hash key
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[B].msQ);
                break;
            case WQ:
                pcon->pieces[W_OCCU]	^= to_64;
                pcon->pieces[WQ]		^= to_64;
                npste->valPcUs          -= valQUEEN;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[W].msQ);
                break;
            case BR:
                pcon->pieces[B_OCCU]	^= to_64;
                pcon->pieces[BR]		^= to_64;
//                npste->castle           &= castle_spoiler[to];
                npste->valPcUs          -= valROOK;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[B].msR);
                break;
            case WR:
                pcon->pieces[W_OCCU]	^= to_64;
                pcon->pieces[WR]		^= to_64;
//                npste->castle           &= castle_spoiler[to];
                npste->valPcUs          -= valROOK;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[W].msR);
                break;
            case BB:
                pcon->pieces[B_OCCU]	^= to_64;
                pcon->pieces[BB]		^= to_64;
                npste->valPcUs          -= valBISHOP;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[B].msB);
                break;
            case WB:
                pcon->pieces[W_OCCU]	^= to_64;
                pcon->pieces[WB]		^= to_64;
                npste->valPcUs          -= valBISHOP;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[W].msB);
                break;
            case BN:
                pcon->pieces[B_OCCU]	^= to_64;
                pcon->pieces[BN]		^= to_64;
                npste->valPcUs          -= valKNIGHT;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[B].msN);
                break;
            case WN:
                pcon->pieces[W_OCCU]	^= to_64;
                pcon->pieces[WN]		^= to_64;
                npste->valPcUs          -= valKNIGHT;
                rehashBB(npste->hashkPc, captured, to);
                msigRemovePiece(captured, pcon->msigBit[W].msN);
                break;
            case BP:
                pcon->pieces[B_OCCU]	^= to_64;
                pcon->pieces[BP]		^= to_64;
                npste->valPnUs          -= valPAWN;
                rehashBB(npste->hashkPc, captured, to);
                rehashBB( npste->hashkPn, captured, to);
                msigRemovePiece(captured, pcon->msigBit[B].msP);
                break;
            case WP:
			    pcon->pieces[W_OCCU]	^= to_64;
			    pcon->pieces[WP]		^= to_64;
                npste->valPnUs          -= valPAWN;
                rehashBB(npste->hashkPc, captured, to);
			    rehashBB( npste->hashkPn, captured, to);
                msigRemovePiece(captured, pcon->msigBit[W].msP);
			    break;
            case B_EP:
                pos[to + 8]		        = 0;
                pcon->pieces[WP]		^= (to_64 << 8);
                pcon->pieces[W_OCCU]	^= (to_64 << 8);
                npste->valPnUs          -= valPAWN;
                rehashBB(npste->hashkPc, captured, to + 8);
                rehashBB(npste->hashkPn, captured, to + 8);
                msigRemovePiece(WP, pcon->msigBit[W].msP);
                break;
            case W_EP:
                pos[to - 8]		        = 0;
                pcon->pieces[BP]		^= (to_64 >> 8);
                pcon->pieces[B_OCCU]	^= (to_64 >> 8);
                npste->valPnUs          -= valPAWN;
                rehashBB(npste->hashkPc, captured, to - 8);
                rehashBB(npste->hashkPn, captured, to - 8);
                msigRemovePiece(BP, pcon->msigBit[B].msP);
                break;
        }
    }
    rehashBB( npste->hashkPc, piece, from );
    rehashBB( npste->hashkPc, piece, to );

    // Move the piece on the bitboards
    switch (Pc(move)) {
		case BK:
//			npste->castle	        &= 3;
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BK]		^= move_64;
			king[B]				    = U8(to);
			break;
		case WK:
//			npste->castle	        &= 12;
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WK]		^= move_64;
			king[W]				    = U8(to);
			break;
		case BQ:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BQ]		^= move_64;
			break;
		case WQ:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WQ]		^= move_64;
			break;
		case BR:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BR]		^= move_64;
//			npste->castle           &= castle_spoiler[from];
			break;
		case WR:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WR]		^= move_64;
//			npste->castle           &= castle_spoiler[from];
			break;
		case BB:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BB]		^= move_64;
			break;
		case WB:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WB]		^= move_64;
			break;
		case BN:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BN]		^= move_64;
			if (knight_attacks[to] & pcon->pieces[WK])
				npste->checkf = 1;
			break;
		case WN:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WN]		^= move_64;
			if (knight_attacks[to] & pcon->pieces[BK])
				npste->checkf = 1;
			break;
		case BP:
            npste->plyFifty         = 0;
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BP]		^= move_64;
            if (from - to == 16 && (pcon->pieces[WP] & pawn_attacks[B][from - 8])) {
				npste->ep_t = U8(from - 8);
                SET_ENPASSANT(npste->hashkPc, npste->ep_t);
            }
			if (pawn_attacks[B][to] & pcon->pieces[WK])
				npste->checkf = 1;
	        rehashBB( npste->hashkPn, piece, from );
	        rehashBB( npste->hashkPn, piece, to );
			break;
		case WP:
            npste->plyFifty         = 0;
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WP]		^= move_64;
            if (to - from == 16 && (pcon->pieces[BP] & pawn_attacks[W][from + 8])) {
				npste->ep_t = U8(from + 8);
                SET_ENPASSANT(npste->hashkPc, npste->ep_t);
            }
			if (pawn_attacks[W][to] & pcon->pieces[BK])
				npste->checkf = 1;
	        rehashBB( npste->hashkPn, piece, from );
	        rehashBB( npste->hashkPn, piece, to );
            Assert(to <= H7);
			break;
		case B_CASTLE:
            // TODO : handle frc cr..
//			npste->castle	        &= 3;
			pcon->pieces[BK]		^= move_64;
			king[B]				    = U8(to);

			if (to == G8) {
				pcon->pieces[B_OCCU]	^= 0xF000000000000000ULL;
				pcon->pieces[BR]		^= 0xA000000000000000ULL;
				pos[F8]			= BR;
				pos[H8]			= 0;
	            rehashBB( npste->hashkPc, BR, H8 );
	            rehashBB( npste->hashkPc, BR, F8 );
			}
			else {
				pcon->pieces[B_OCCU]	^= 0x1D00000000000000ULL;
				pcon->pieces[BR]		^= 0x0900000000000000ULL;
				pos[A8]			= 0;
				pos[D8]			= BR;
	            rehashBB( npste->hashkPc, BR, A8 );
	            rehashBB( npste->hashkPc, BR, D8 );
			}
			break;
		case W_CASTLE:
//			npste->castle	        &= 12;
			pcon->pieces[WK]		^= move_64;
			king[W]				    = U8(to);

			if (to == G1) {
				pcon->pieces[W_OCCU]	^= 0x00000000000000F0ULL;
				pcon->pieces[WR]		^= 0x00000000000000A0ULL;
				pos[F1]			= WR;
				pos[H1]			= 0;
	            rehashBB( npste->hashkPc, WR, H1 );
	            rehashBB( npste->hashkPc, WR, F1 );
			}
			else {
				pcon->pieces[W_OCCU]	^= 0x000000000000001DULL;
				pcon->pieces[WR]		^= 0x0000000000000009ULL;
				pos[A1]			= 0;
				pos[D1]			= WR;
	            rehashBB( npste->hashkPc, WR, A1 );
	            rehashBB( npste->hashkPc, WR, D1 );
			}
			break;
		case B_PROM:
			pcon->pieces[BP]			^= from_64;
			pcon->pieces[B_OCCU]		^= move_64;
            npste->plyFifty             = 0;
            npste->valPnThem            -= valPAWN;
            // delete the pawn
            rehashBB( npste->hashkPc, BP, from );
            rehashBB( npste->hashkPn, BP, from );
            msigRemovePiece(BP, pcon->msigBit[B].msP);

			switch (Prom(move)) {
				case PROM_Q:
					pcon->pieces[BQ]	^= to_64;
					pos[to]		        = BQ;
                    npste->valPcThem      += valQUEEN;
                    // add a piece
                    rehashBB( npste->hashkPc, BQ, to);
                    msigAddPiece(BQ, pcon->msigBit[B].msQ);
					break;
				case PROM_R:
					pcon->pieces[BR]	^= to_64;
					pos[to]		        = BR;
                    npste->valPcThem      += valROOK;
                    rehashBB( npste->hashkPc, BR, to);
                    msigAddPiece(BR, pcon->msigBit[B].msR);
					break;
				case PROM_B:
					pcon->pieces[BB]	^= to_64;
					pos[to]		        = BB;
                    npste->valPcThem      += valBISHOP;
                    rehashBB( npste->hashkPc, BB, to);
                    msigAddPiece(BB, pcon->msigBit[B].msB);
					break;
				case PROM_N:
					pcon->pieces[BN]	^= to_64;
					pos[to]		        = BN;
                    npste->valPcThem      += valKNIGHT;
                    rehashBB( npste->hashkPc, BN, to);
                    msigAddPiece(BN, pcon->msigBit[B].msN);
					if (knight_attacks[to] & pcon->pieces[WK])
						npste->checkf = 1;
					break;
                default:
                    Assert(0);
                    break;
			}
			break;
		case W_PROM:
			pcon->pieces[WP]			^= from_64;
			pcon->pieces[W_OCCU]		^= move_64;
            npste->plyFifty             = 0;
            npste->valPnThem              -= valPAWN;
            // delete the pawn
            rehashBB( npste->hashkPc, WP, from );
            rehashBB( npste->hashkPn, WP, from );
            msigRemovePiece(WP, pcon->msigBit[W].msP);

			switch (Prom(move)) {
				case PROM_Q:
					pcon->pieces[WQ]	^= to_64;
					pos[to]		        = WQ;
                    npste->valPcThem      += valQUEEN;
                    // add a piece
                    rehashBB( npste->hashkPc, WQ, to);
                    msigAddPiece(WQ, pcon->msigBit[W].msQ);
					break;
				case PROM_R:
					pcon->pieces[WR]	^= to_64;
					pos[to]		        = WR;
                    npste->valPcThem      += valROOK;
                    rehashBB( npste->hashkPc, WR, to);
                    msigAddPiece(WR, pcon->msigBit[W].msR);
					break;
				case PROM_B:
					pcon->pieces[WB]	^= to_64;
					pos[to]		        = WB;
                    npste->valPcThem      += valBISHOP;
                    rehashBB( npste->hashkPc, WB, to);
                    msigAddPiece(WB, pcon->msigBit[W].msB);
					break;
				case PROM_N:
					pcon->pieces[WN]	^= to_64;
					pos[to]		        = WN;
                    npste->valPcThem      += valKNIGHT;
                    rehashBB( npste->hashkPc, WN, to);
                    msigAddPiece(WN, pcon->msigBit[W].msN);
					if (knight_attacks[to] & pcon->pieces[BK])
						npste->checkf = 1;
					break;
                default:
                    Assert(0);
                    break;
			}
			break;
	}
    pcon->gc.arghashk[++pcon->gc.ccm] = npste->hashkPc;

    pcon->pieces[OCCU] = pcon->pieces[B_OCCU] | pcon->pieces[W_OCCU];
	pcon->pieces[X_OCCU] = ~pcon->pieces[OCCU];
#ifdef _DEBUG
    checkBoard(pcon,npste);
//	int pking = king[npste->side];
//    if (Attacked(pcon, pking, npste->side^1))
//        Assert(false);

    int v = 0;
    for(int i = 0; i < 12 ; i++ )
        if( (i & 1) != npste->side )
            v += pcon->msigCount[i] * b_valPc[i];
        else
            v -= pcon->msigCount[i] * b_valPc[i];
    if( v != (npste->valPcThem + npste->valPnThem - npste->valPcUs - npste->valPnUs ) ) {
        Assert(0);
    }
    // verify that we don't put our king in check
    Assert(!Attacked(pcon, pcon->king[npste->side^1], npste->side));
#endif

	PinCheckUpdate(pcon, npste, npste->side);
}

void bNullMove(PCON pcon, PSTE pste)
{
    STE * RESTRICT npste = pste + 1;  // next stack element
    pste->lastMove = 0;     // to display the current variation

    // Update the current status and ply
//    npste->checkdone[B] = pste->checkdone[B];
//    npste->checkdone[W] = pste->checkdone[W];
    npste->castle   = pste->castle;     // Castling flags are passed.
    npste->ep_t     = 0;
    npste->checkf   = 0;
    npste->side     = pste->side ^ 1;
    npste->xside    = pste->side;

    npste->fNull    = true;             // Null move not allowed.
//    npste->c_ttpair = pste->c_ttpair;
    npste->evaluated= false;
//    npste->danger   = false;

    npste->mlist_start = pste->mlist_start;
    //
    //	Initialize various values for the next ply.
    //
    npste->plyFifty = 0;
    npste->hashkPc  = HashkSwitch(pste->hashkPc);

    if( pste->ep_t )
        SET_ENPASSANT(npste->hashkPc, pste->ep_t);
    npste->hashkPn  = pste->hashkPn;
    npste->valPcUs  = pste->valPcThem;
    npste->valPcThem= pste->valPcUs;
    npste->valPnUs  = pste->valPnThem;
    npste->valPnThem= pste->valPnUs;
//    npste->prevCaptValue    = 0;
    npste->prevCaptSqr      = -1;   // HACK

    pcon->gc.arghashk[++pcon->gc.ccm] = npste->hashkPc;

    PinCheckUpdate(pcon, npste, npste->side);

}


void bUndoMove(PCON pcon, PSTE pste, U32 move)
{
    U8 *pos = pcon->pos;
    unsigned char *king = pcon->king;

    //    checkKing(pcon);
    // Decompress the move
    int from    = From(move);
    int to      = To(move);
    int pc      = Pc(move);
    int capt    = Capt(move);

    bitboard to_64      = IdxToU64(to);
    bitboard from_64    = IdxToU64(from);
    bitboard move_64    = to_64 | from_64;

    // Move the piece on the array board
    pos[from]   = U8(pc);
    pos[to]     = capt;

    // Perform captures, if any
    switch (capt) {
		case BQ:
			pcon->pieces[B_OCCU]	^= to_64;
			pcon->pieces[BQ]		^= to_64;
            msigAddPiece(BQ, pcon->msigBit[B].msQ);
			break;
		case WQ:
			pcon->pieces[W_OCCU]	^= to_64;
			pcon->pieces[WQ]		^= to_64;
            msigAddPiece(WQ, pcon->msigBit[W].msQ);
			break;
		case BR:
			pcon->pieces[B_OCCU]		^= to_64;
			pcon->pieces[BR]		^= to_64;
            msigAddPiece(BR, pcon->msigBit[B].msR);
			break;
		case WR:
			pcon->pieces[W_OCCU]	^= to_64;
			pcon->pieces[WR]		^= to_64;
            msigAddPiece(WR, pcon->msigBit[W].msR);
			break;
		case BB:
			pcon->pieces[B_OCCU]	^= to_64;
			pcon->pieces[BB]		^= to_64;
            msigAddPiece(BB, pcon->msigBit[B].msB);
			break;
		case WB:
			pcon->pieces[W_OCCU]	^= to_64;
			pcon->pieces[WB]		^= to_64;
            msigAddPiece(WB, pcon->msigBit[W].msB);
			break;
		case BN:
			pcon->pieces[B_OCCU]	^= to_64;
			pcon->pieces[BN]		^= to_64;
            msigAddPiece(BN, pcon->msigBit[B].msN);
			break;
		case WN:
			pcon->pieces[W_OCCU]	^= to_64;
			pcon->pieces[WN]		^= to_64;
            msigAddPiece(WN, pcon->msigBit[W].msN);
			break;
		case BP:
			pcon->pieces[B_OCCU]	^= to_64;
			pcon->pieces[BP]		^= to_64;
            msigAddPiece(BP, pcon->msigBit[B].msP);
			break;
		case WP:
			pcon->pieces[W_OCCU]	^= to_64;
			pcon->pieces[WP]		^= to_64;
            msigAddPiece(WP, pcon->msigBit[W].msP);
			break;
		case B_EP:
			pos[to]			= 0;
			pos[to + 8]		= WP;
			pcon->pieces[WP]		^= (to_64 << 8);
			pcon->pieces[W_OCCU]	^= (to_64 << 8);
            msigAddPiece(WP, pcon->msigBit[W].msP);
			break;
		case W_EP:
			pos[to]			= 0;
			pos[to - 8]		= BP;
			pcon->pieces[BP]		^= (to_64 >> 8);
			pcon->pieces[B_OCCU]	^= (to_64 >> 8);
            msigAddPiece(BP, pcon->msigBit[B].msP);
	}

    // Move the piece on the bitboards
    switch (pc) {
        case BK:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BK]		^= move_64;
			king[B]			= U8(from);
			break;
		case WK:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WK]		^= move_64;
			king[W]			= U8(from);
			break;
		case BQ:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BQ]		^= move_64;
			break;
		case WQ:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WQ]		^= move_64;
			break;
		case BR:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BR]		^= move_64;
			break;
		case WR:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WR]		^= move_64;
			break;
		case BB:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BB]		^= move_64;
			break;
		case WB:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WB]		^= move_64;
			break;
		case BN:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BN]		^= move_64;
			break;
		case WN:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WN]		^= move_64;
			break;
		case BP:
			pcon->pieces[B_OCCU]	^= move_64;
			pcon->pieces[BP]		^= move_64;
			break;
		case WP:
			pcon->pieces[W_OCCU]	^= move_64;
			pcon->pieces[WP]		^= move_64;
			break;
		case B_CASTLE:
			pcon->pieces[BK]		^= move_64;
			king[B]			= U8(from);

			if (to == G8) {
				pcon->pieces[B_OCCU]	^= 0xF000000000000000ULL;
				pcon->pieces[BR]		^= 0xA000000000000000ULL;
				pos[E8]			= BK;
				pos[F8]			= 0;
				pos[H8]			= BR;
			}
			else {
				pcon->pieces[B_OCCU]	^= 0x1D00000000000000ULL;
				pcon->pieces[BR]		^= 0x0900000000000000ULL;
				pos[A8]			= BR;
				pos[D8]			= 0;
				pos[E8]			= BK;
			}
			break;
		case W_CASTLE:
			pcon->pieces[WK]			^= move_64;
			king[W]				= U8(from);

			if (to == G1) {
				pcon->pieces[W_OCCU]	^= 0xF0ULL;
				pcon->pieces[WR]		^= 0xA0ULL;
				pos[E1]			= WK;
				pos[F1]			= 0;
				pos[H1]			= WR;
			}
			else {
				pcon->pieces[W_OCCU]	^= 0x1DULL;
				pcon->pieces[WR]		^= 0x9ULL;
				pos[A1]			= WR;
				pos[D1]			= 0;
				pos[E1]			= WK;
			}
			break;
		case B_PROM:
			pcon->pieces[BP]		^= from_64;
			pcon->pieces[B_OCCU]	^= move_64;
			pos[from]		= BP;
            msigAddPiece(BP, pcon->msigBit[B].msP);

			switch (Prom(move)) {
				case PROM_Q:
					pcon->pieces[BQ]	^= to_64;
                    msigRemovePiece(BQ, pcon->msigBit[B].msQ);
					break;
				case PROM_R:
					pcon->pieces[BR]	^= to_64;
                    msigRemovePiece(BR, pcon->msigBit[B].msR);
					break;
				case PROM_B:
					pcon->pieces[BB]	^= to_64;
                    msigRemovePiece(BB, pcon->msigBit[B].msB);
					break;
				case PROM_N:
					pcon->pieces[BN]	^= to_64;
                    msigRemovePiece(BN, pcon->msigBit[B].msN);
					break;
			}
			break;
		case W_PROM:
			pcon->pieces[WP]			^= from_64;
			pcon->pieces[W_OCCU]		^= move_64;
			pos[from]			= WP;
            msigAddPiece(WP, pcon->msigBit[W].msP);

			switch (Prom(move)) {
				case PROM_Q:
					pcon->pieces[WQ]	^= to_64;
                    msigRemovePiece(WQ, pcon->msigBit[W].msQ);
					break;
				case PROM_R:
					pcon->pieces[WR]	^= to_64;
                    msigRemovePiece(WR, pcon->msigBit[W].msR);
					break;
				case PROM_B:
					pcon->pieces[WB]	^= to_64;
                    msigRemovePiece(WB, pcon->msigBit[W].msB);
					break;
				case PROM_N:
					pcon->pieces[WN]	^= to_64;
                    msigRemovePiece(WN, pcon->msigBit[W].msN);
					break;
			}
			break;
	}
#ifdef _DEBUG
    int v = 0;
    for(int i = 0; i < 12 ; i++ )
        if( (i & 1) != pste->side )
            v += pcon->msigCount[i] * b_valPc[i];
        else
            v -= pcon->msigCount[i] * b_valPc[i];
    if( v != (pste->valPcThem + pste->valPnThem - pste->valPcUs - pste->valPnUs ) ) {
        Assert(0);
    }

#endif

	pcon->pieces[OCCU] = pcon->pieces[B_OCCU] | pcon->pieces[W_OCCU];
	pcon->pieces[X_OCCU] = ~pcon->pieces[OCCU];
    --pcon->gc.ccm;

//    checkKing(pcon);
}
void bUndoNullMove(PCON pcon, PSTE pste) {
    --pcon->gc.ccm;
}




// Initializes and begins an endless perft loop. It can accept a fen string
// (no quotations, etc.) as an argument, and _will_ crash on invalid fens
int testMAGIC(PCON pcon, PSTE pste, int depth)
{
	int i;
	clock_t start, end;

//	SetFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

//	SetFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

//	if (fen != 0) {
//		SetFromFEN(fen);
//	}

	Print(pcon, pste);

	// Begin an infinite perft loop
	for (i = 1; i <= depth; i++) {
		nodes = 0;

		start = clock();

		Perft(pcon, pste, i);

		end = clock();

		float dtime = (float(end - start) / CLOCKS_PER_SEC);

		printf("%2d. %10d %7.2f\n", i, int(nodes), dtime);
	}

	return 0;
}
static struct {
    const char *fen;
    int depth;
    unsigned int num_moves;
} position_list[] = {
    {"8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 3, 2816},
    {"K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1",3, 1416},
    {"K7/8/8/3Q4/4q3/8/8/7k w - - 0 1",4, 8349},
    {"8/8/8/8/8/K7/P7/k7 b - - 0 1 ", 6, 2343},
    {"8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1",5, 23599},
    {"n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 3, 7421},
    {"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 3, 9483},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902},
    {"8/3K4/2p5/p2b2r1/5k2/8/8/1q6 b - - 1 67", 2, 279},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039},
    {"4k3/8/8/8/8/8/8/4K2R w K - 0 1",4,7059},
    {"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",3,13744},
    {"3rk2r/p4pRp/4p3/q1pPn3/1pP5/4p2B/PQ2KP1P/2R5 b - - 0 1", 3, 47351},
    {"8/7p/2k1Pp2/pp1p2p1/3P2P1/4P3/P3K2P/8 w - - 0 1 ", 4, 15233},
    {"1n1k1br1/1pqbnQ2/r2N3p/p1p1B1p1/8/P6P/BPP2PP1/3R1RK1 w - - 0 1", 3, 95937},
};

void verify_move_gen(PCON pcon, PSTE pste) {
    bool ok = true;
	clock_t start = clock();
    for(unsigned int i = 0; i < sizeof(position_list) / sizeof(position_list[0]) ; i++) {
        if( SetBoardFromFEN(pcon, position_list[i].fen) ) {
            nodes = 0;
            printf("# %d %7d %s", position_list[i].depth, position_list[i].num_moves, position_list[i].fen);
		    Perft(pcon, pste, position_list[i].depth );
            printf("%7d\n", int(nodes));
            if( nodes != position_list[i].num_moves ) {
                ok = false;
                printf("# perft error %s depth(%d) expected num_moves(%d) got(%d)\n", position_list[i].fen,
                    position_list[i].depth, position_list[i].num_moves, int(nodes));
                Assert(false);
            }
        }
    }
    clock_t end = clock();
	float dtime = (float(end - start) / CLOCKS_PER_SEC);
    printf("# perft time %7.2f %s\n", dtime, ok ? "perft OK" : "***** error in movegen *****");
}

#if 0
#define USE_32_BIT_MULTIPLICATIONS

#include <stdlib.h>

//typedef unsigned long long uint64;
typedef U64 uint64;

uint64 random_uint64() {
  uint64 u1, u2, u3, u4;
  u1 = (uint64)(rand()) & 0xFFFF; u2 = (uint64)(rand()) & 0xFFFF;
  u3 = (uint64)(rand()) & 0xFFFF; u4 = (uint64)(rand()) & 0xFFFF;
  return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

uint64 random_uint64_fewbits() {
  return random_uint64() & random_uint64() & random_uint64();
}

int count_1s(uint64 b) {
  int r;
  for(r = 0; b; r++, b &= b - 1);
  return r;
}

const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

static int pop_1st_bit(Bitboard* b) {
  Bitboard bb = *b ^ (*b - 1);
  unsigned int fold = int(bb) ^ int(bb >> 32);
  *b &= (*b - 1);
  return BitTable[(fold * 0x783a9b23) >> 26];
}
/*
int pop_1st_bit(uint64 *bb) {
  uint64 b = *bb ^ (*bb - 1);
  unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
  *bb &= (*bb - 1);
  return BitTable[(fold * 0x783a9b23) >> 26];
}
*/

static uint64 index_to_uint64(int index, int bits, uint64 m) {
  int i, j;
  uint64 result = 0ULL;
  for(i = 0; i < bits; i++) {
    j = pop_1st_bit(&m);
    if(index & (1 << i)) result |= (1ULL << j);
  }
  return result;
}

static uint64 rmask(int sq) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
  for(r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
  for(f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
  for(f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
  return result;
}

static uint64 bmask(int sq) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r=rk+1, f=fl+1; r<=6 && f<=6; r++, f++) result |= (1ULL << (f + r*8));
  for(r=rk+1, f=fl-1; r<=6 && f>=1; r++, f--) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl+1; r>=1 && f<=6; r--, f++) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl-1; r>=1 && f>=1; r--, f--) result |= (1ULL << (f + r*8));
  return result;
}

static uint64 ratt(int sq, uint64 block) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 7; r++) {
    result |= (1ULL << (fl + r*8));
    if(block & (1ULL << (fl + r*8))) break;
  }
  for(r = rk-1; r >= 0; r--) {
    result |= (1ULL << (fl + r*8));
    if(block & (1ULL << (fl + r*8))) break;
  }
  for(f = fl+1; f <= 7; f++) {
    result |= (1ULL << (f + rk*8));
    if(block & (1ULL << (f + rk*8))) break;
  }
  for(f = fl-1; f >= 0; f--) {
    result |= (1ULL << (f + rk*8));
    if(block & (1ULL << (f + rk*8))) break;
  }
  return result;
}

static uint64 batt(int sq, uint64 block) {
  uint64 result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
    result |= (1ULL << (f + r*8));
    if(block & (1ULL << (f + r * 8))) break;
  }
  return result;
}


static int transform(uint64 b, uint64 magic, int bits) {
#if defined(USE_32_BIT_MULTIPLICATIONS)
  return
    (unsigned)((int)b*(int)magic ^ (int)(b>>32)*(int)(magic>>32)) >> (32-bits);
#else
  return (int)((b * magic) >> (64 - bits));
#endif
}

static uint64 find_magic(int sq, int m, int bishop) {
  uint64 mask, b[4096], a[4096], used[4096], magic;
  int i, j, k, n, fail;

  mask = bishop? bmask(sq) : rmask(sq);
  n = count_1s(mask);

  for(i = 0; i < (1 << n); i++) {
    b[i] = index_to_uint64(i, n, mask);
    a[i] = bishop? batt(sq, b[i]) : ratt(sq, b[i]);
  }
  for(k = 0; k < 100000000; k++) {
    magic = random_uint64_fewbits();
    if(count_1s((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
    for(i = 0; i < 4096; i++) used[i] = 0ULL;
    for(i = 0, fail = 0; !fail && i < (1 << n); i++) {
      j = transform(b[i], magic, m);
      if(used[j] == 0ULL) used[j] = a[i];
      else if(used[j] != a[i]) fail = 1;
    }
    if(!fail) return magic;
  }
  printf("***Failed***\n");
  return 0ULL;
}

static int RBits[64] = {
  12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12
};

static int BBits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6
};

int genmagic2() {
  int square;

  printf("const uint64 RMagic[64] = {\n");
  for(square = 0; square < 64; square++)  {
    printf("  0x%llxULL,", find_magic(square, RBits[square], 0));
    if((square & 3) == 3)
        printf("\n");
  }
  printf("};\n\n");

  printf("const uint64 BMagic[64] = {\n");
  for(square = 0; square < 64; square++) {
    printf("  0x%llxULL,", find_magic(square, BBits[square], 1));
    if((square & 3) == 3)
        printf("\n");
  }
  printf("};\n\n");

  return 0;
}
#endif
