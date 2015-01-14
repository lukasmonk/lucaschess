
#ifndef _GENMAGIC_HPP_
#define _GENMAGIC_HPP_

#include "config.hpp"

#ifdef TARGET64
    #define IdxToU64(idx)   (1ULL << (idx))
#else
    #define IdxToU64(idx)   (_IdxToU64[idx])
#endif

#define Idx(rank, file)		((file) | ((rank) << 3))

#define Set(bit, val)	(val |= (1ULL << bit))
#define Clear(bit, val) (val &= ~(1ULL << bit))
#define IsSet(bit, val) ((val >> bit) & 1)

extern const int lsz64_tbl[64];
extern U64 pawn_attacks[2][64];
extern U64 king_attacks[64];
extern U64 king_attacks_ring[64];
extern U64 knight_attacks[64];
extern U64 knight_future_attacks[64];
extern U64 _IdxToU64[64];
extern const int woccu_wp[2];
extern const int boccu_bp[2];
extern const U64 xprom_mask[2];

// Pawn move generation data
extern const int xoccu_wp[2];
extern const int xoccu_bp[2];
extern const int xoccu_tmp[2];
extern const U64 prom_mask[2];

const U64 CLEAR_LEFT	= 0xFEFEFEFEFEFEFEFEULL;
const U64 CLEAR_RIGHT	= 0x7F7F7F7F7F7F7F7FULL;

const U64 notAFile = C64(0xfefefefefefefefe); // ~0x0101010101010101
const U64 notHFile = C64(0x7f7f7f7f7f7f7f7f); // ~0x8080808080808080
const bitboard rank2Mask = 0xFF00ULL;
const bitboard rank7Mask = 0xFF000000000000ULL;
const bitboard rank1Mask = 0xFFULL;
const bitboard rank8Mask = 0xFF00000000000000ULL;

// see http://chessprogramming.wikispaces.com/
// thx Gerd !
INLINE U64 soutOne (U64 b) {return  b >> 8;}
INLINE U64 nortOne (U64 b) {return  b << 8;}
INLINE U64 eastOne (U64 b) {return (b  * 2) & notAFile;}
INLINE U64 noEaOne (U64 b) {return (b << 9) & notAFile;}
INLINE U64 soEaOne (U64 b) {return (b >> 7) & notAFile;}
INLINE U64 westOne (U64 b) {return (b >> 1) & notHFile;}
INLINE U64 soWeOne (U64 b) {return (b >> 9) & notHFile;}
INLINE U64 noWeOne (U64 b) {return (b << 7) & notHFile;}
INLINE U64 nortFill(U64 gen) {
    gen |= (gen <<  8);
    gen |= (gen << 16);
    gen |= (gen << 32);
    return gen;
}
 INLINE U64 soutFill(U64 gen) {
    gen |= (gen >>  8);
    gen |= (gen >> 16);
    gen |= (gen >> 32);
    return gen;
}
INLINE U64 wFrontSpans(U64 wpawns) {return nortOne (nortFill(wpawns));}
INLINE U64 bFrontSpans(U64 bpawns) {return soutOne (soutFill(bpawns));}

static U64 flipVertical(U64 x) {
   const U64 k1 = C64(0x00FF00FF00FF00FF);
   const U64 k2 = C64(0x0000FFFF0000FFFF);
   const U64 k4 = C64(0x00000000FFFFFFFF);
   x = ((x >>  8) & k1) | ((x & k1) <<  8);
   x = ((x >> 16) & k2) | ((x & k2) << 16);
   x = ((x >> 32) & k4) | ((x & k4) << 32);
   return x;
}


/*INLINE int  bitScan (const U64 bb)
{
   const U64 lsb = (bb & -(long long) bb) - 1;
   const unsigned int foldedLSB = ((int) lsb) ^ ((int) (lsb >> 32));
   return lsz64_tbl[foldedLSB * 0x78291ACF >> 26];
}*/

INLINE int bitScan(const bitboard & bb)
{
    const bitboard  lsb = (bb & -(long long) bb) - 1;
    const unsigned int foldedLSB = (unsigned) ((lsb & 0xffffffff) ^ (lsb >> 32));
    return lsz64_tbl[foldedLSB * 0x78291ACF >> 26];
}

INLINE unsigned int bitScanAndReset(bitboard & bb)
{
    bitboard        b = bb ^ (bb - 1);
    bb &= (bb - 1);
    unsigned int    fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
    return lsz64_tbl[(fold * 0x78291ACF) >> (32 - 6)];
}

#ifndef LSB_DEFINED
#define LSB_DEFINED
	INLINE unsigned int LSB (const bitboard bb)
	{
		return bitScan( bb );
//		return index64[((bb & -bb) * debruijn64) >> 58];
	}
#endif


INLINE unsigned int PopLSB(bitboard & bb)
{
//	return bitScanAndReset( bb );
	unsigned int lsb = LSB( bb );
    bb &= (bb - 1);
	return lsb;
}

INLINE int lowpopCount (U64 x) {
   int count = 0;
   U32 y = U32(x);
   while (y) {
       count++;
       y &= y - 1; // reset LS1B
   }
   y = U32(x>>32);
   while (y) {
       count++;
       y &= y - 1; // reset LS1B
   }
   return count;
}

extern unsigned popCount1(bitboard b);
extern unsigned popCount2(const bitboard b);
#define popCount    popCount2

//extern void AddPiece(PCON pcon, int idx, int piece);
extern void InitBitboards(void);
extern void convMailboxtoBB(PCON pcon, PSTE pste);

extern void verify_move_gen(PCON pcon, PSTE pste);

#endif //_GENMAGIC_HPP_
