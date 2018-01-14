#include "chess.h"
#include "randoms.h"

// static member variable definations
ZobristRandoms BitBoardUtils::zob;

// bit mask containing squares between two given squares
uint64 BitBoardUtils::Between[64][64];

// bit mask containing squares in the same 'line' as two given squares
uint64 BitBoardUtils::Line[64][64];

// squares a piece can attack in an empty board
uint64 BitBoardUtils::RookAttacks[64];
uint64 BitBoardUtils::BishopAttacks[64];
uint64 BitBoardUtils::QueenAttacks[64];
uint64 BitBoardUtils::KingAttacks[64];
uint64 BitBoardUtils::KnightAttacks[64];
uint64 BitBoardUtils::pawnAttacks[2][64];

// magic lookup tables
// plain magics (Fancy magic lookup tables in FancyMagics.h)
uint64 BitBoardUtils::rookMagics[64];
uint64 BitBoardUtils::bishopMagics[64];

// same as RookAttacks and BishopAttacks, but corner bits masked off
uint64 BitBoardUtils::RookAttacksMasked[64];
uint64 BitBoardUtils::BishopAttacksMasked[64];

uint64   BitBoardUtils::rookMagicAttackTables[64][1 << ROOK_MAGIC_BITS];    // 2 MB
uint64 BitBoardUtils::bishopMagicAttackTables[64][1 << BISHOP_MAGIC_BITS];  // 256 KB



uint8 BitBoardUtils::popCount(uint64 x)
{
#if USE_POPCNT == 1
#ifdef __CUDA_ARCH__
    return __popcll(x);
#elif __GNUC__
    return __builtin_popcountll(x);
#elif defined(_WIN64)
    return _mm_popcnt_u64(x);
#else
    uint32 lo = (uint32)x;
    uint32 hi = (uint32)(x >> 32);
    return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
#endif
#else

    // taken from chess prgramming wiki: http://chessprogramming.wikispaces.com/Population+Count
    const uint64 k1 = C64(0x5555555555555555); /*  -1/3   */
    const uint64 k2 = C64(0x3333333333333333); /*  -1/5   */
    const uint64 k4 = C64(0x0f0f0f0f0f0f0f0f); /*  -1/17  */
    const uint64 kf = C64(0x0101010101010101); /*  -1/255 */

    x = x - ((x >> 1)  & k1); /* put count of each 2 bits into those 2 bits */
    x = (x & k2) + ((x >> 2)  & k2); /* put count of each 4 bits into those 4 bits */
    x = (x + (x >> 4)) & k4; /* put count of each 8 bits into those 8 bits */
    x = (x * kf) >> 56;              /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */

    return (uint8)x;
#endif
}

uint8 BitBoardUtils::bitScan(uint64 x)
{
#if USE_HW_BITSCAN == 1
#ifdef __CUDA_ARCH__
    // __ffsll(x) returns position from 1 to 64 instead of 0 to 63
    return __ffsll(x) - 1;
#elif __GNUC__
    return __builtin_ffsll(x) - 1;
#elif _WIN64
    unsigned long index = 0;
    assert(x != 0);

    _BitScanForward64(&index, x);
    return (uint8)index;
#else

    unsigned long lo = (unsigned long)x;
    unsigned long hi = (unsigned long)(x >> 32);
    unsigned long id;

    if (lo)
        _BitScanForward(&id, lo);
    else
    {
        _BitScanForward(&id, hi);
        id += 32;
    }

    return (uint8)id;
#endif
#else
const int index64[64] = {
        0, 1, 48, 2, 57, 49, 28, 3,
        61, 58, 50, 42, 38, 29, 17, 4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12, 5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19, 9, 13, 8, 7, 6
    };
    const uint64 debruijn64 = C64(0x03f79d71b4cb0a89);
    assert(x != 0);
    return index64[((x & -static_cast<int64>(x)) * debruijn64) >> 58];
#endif
}


// gets one bit (the LSB) from a bitboard
// returns a bitboard containing that bit
uint64 BitBoardUtils::getOne(uint64 x)
{
    return x & (-static_cast<int64>(x));
}


bool BitBoardUtils::isMultiple(uint64 x)
{
    return x ^ getOne(x);
}

bool BitBoardUtils::isSingular(uint64 x)
{
    return !isMultiple(x);
}

// lookup table helper functions
uint64 BitBoardUtils::sqsInBetweenLUT(uint8 sq1, uint8 sq2)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gBetween[sq1][sq2]);
#else
    return Between[sq1][sq2];
#endif
}

uint64 BitBoardUtils::sqsInLineLUT(uint8 sq1, uint8 sq2)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gLine[sq1][sq2]);
#else
    return Line[sq1][sq2];
#endif
}

uint64 BitBoardUtils::sqKnightAttacks(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gKnightAttacks[sq]);
#else
    return KnightAttacks[sq];
#endif
}

uint64 BitBoardUtils::sqKingAttacks(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gKingAttacks[sq]);
#else
    return KingAttacks[sq];
#endif
}

uint64 BitBoardUtils::sqRookAttacks(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gRookAttacks[sq]);
#else
    return RookAttacks[sq];
#endif
}

uint64 BitBoardUtils::sqBishopAttacks(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gBishopAttacks[sq]);
#else
    return BishopAttacks[sq];
#endif
}

uint64 BitBoardUtils::sqBishopAttacksMasked(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gBishopAttacksMasked[sq]);
#else
    return BishopAttacksMasked[sq];
#endif
}

uint64 BitBoardUtils::sqRookAttacksMasked(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gRookAttacksMasked[sq]);
#else
    return RookAttacksMasked[sq];
#endif
}

uint64 BitBoardUtils::sqRookMagics(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gRookMagics[sq]);
#else
    return rookMagics[sq];
#endif
}

uint64 BitBoardUtils::sqBishopMagics(uint8 sq)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gBishopMagics[sq]);
#else
    return bishopMagics[sq];
#endif
}

uint64 BitBoardUtils::sqRookMagicAttackTables(uint8 sq, uint64 index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gRookMagicAttackTables[sq][index]);
#else
    return rookMagicAttackTables[sq][index];
#endif
}

uint64 BitBoardUtils::sqBishopMagicAttackTables(uint8 sq, uint64 index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&gBishopMagicAttackTables[sq][index]);
#else
    return bishopMagicAttackTables[sq][index];
#endif
}

uint64 BitBoardUtils::sq_fancy_magic_lookup_table(int index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&g_fancy_magic_lookup_table[index]);
#else
    return fancy_magic_lookup_table[index];
#endif
}

FancyMagicEntry BitBoardUtils::sq_bishop_magics_fancy(int sq)
{
#ifdef __CUDA_ARCH__
    FancyMagicEntry op;
    op.data = __ldg(&(((uint4 *)g_bishop_magics_fancy)[sq]));
    return op;
#else
    return bishop_magics_fancy[sq];
#endif
}

FancyMagicEntry BitBoardUtils::sq_rook_magics_fancy(int sq)
{
#ifdef __CUDA_ARCH__
    FancyMagicEntry op;
    op.data = __ldg(&(((uint4 *)g_rook_magics_fancy)[sq]));
    return op;
#else
    return rook_magics_fancy[sq];
#endif
}

uint8 BitBoardUtils::sq_fancy_byte_magic_lookup_table(int index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&g_fancy_byte_magic_lookup_table[index]);
#else
    return fancy_byte_magic_lookup_table[index];
#endif
}

uint64 BitBoardUtils::sq_fancy_byte_BishopLookup(int index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&g_fancy_byte_BishopLookup[index]);
#else
    return fancy_byte_BishopLookup[index];
#endif
}

uint64 BitBoardUtils::sq_fancy_byte_RookLookup(int index)
{
#ifdef __CUDA_ARCH__
    return __ldg(&g_fancy_byte_RookLookup[index]);
#else
    return fancy_byte_RookLookup[index];
#endif
}



// move the bits in the bitboard one square in the required direction
uint64 BitBoardUtils::northOne(uint64 x)
{
    return x << 8;
}

uint64 BitBoardUtils::southOne(uint64 x)
{
    return x >> 8;
}

uint64 BitBoardUtils::eastOne(uint64 x)
{
    return (x << 1) & (~FILEA);
}

uint64 BitBoardUtils::westOne(uint64 x)
{
    return (x >> 1) & (~FILEH);
}

uint64 BitBoardUtils::northEastOne(uint64 x)
{
    return (x << 9) & (~FILEA);
}

uint64 BitBoardUtils::northWestOne(uint64 x)
{
    return (x << 7) & (~FILEH);
}

uint64 BitBoardUtils::southEastOne(uint64 x)
{
    return (x >> 7) & (~FILEA);
}

uint64 BitBoardUtils::southWestOne(uint64 x)
{
    return (x >> 9) & (~FILEH);
}


// fill the board in the given direction
// taken from http://chessprogramming.wikispaces.com/


// gen - generator  : starting positions
// pro - propogator : empty squares / squares not of current side

// uses kogge-stone algorithm

uint64 BitBoardUtils::northFill(uint64 gen, uint64 pro)
{
    gen |= (gen << 8) & pro;
    pro &= (pro << 8);
    gen |= (gen << 16) & pro;
    pro &= (pro << 16);
    gen |= (gen << 32) & pro;

    return gen;
}

uint64 BitBoardUtils::southFill(uint64 gen, uint64 pro)
{
    gen |= (gen >> 8) & pro;
    pro &= (pro >> 8);
    gen |= (gen >> 16) & pro;
    pro &= (pro >> 16);
    gen |= (gen >> 32) & pro;

    return gen;
}

uint64 BitBoardUtils::eastFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen << 1) & pro;
    pro &= (pro << 1);
    gen |= (gen << 2) & pro;
    pro &= (pro << 2);
    gen |= (gen << 3) & pro;

    return gen;
}

uint64 BitBoardUtils::westFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen >> 1) & pro;
    pro &= (pro >> 1);
    gen |= (gen >> 2) & pro;
    pro &= (pro >> 2);
    gen |= (gen >> 3) & pro;

    return gen;
}


uint64 BitBoardUtils::northEastFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen << 9) & pro;
    pro &= (pro << 9);
    gen |= (gen << 18) & pro;
    pro &= (pro << 18);
    gen |= (gen << 36) & pro;

    return gen;
}

uint64 BitBoardUtils::northWestFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen << 7) & pro;
    pro &= (pro << 7);
    gen |= (gen << 14) & pro;
    pro &= (pro << 14);
    gen |= (gen << 28) & pro;

    return gen;
}

uint64 BitBoardUtils::southEastFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen >> 7) & pro;
    pro &= (pro >> 7);
    gen |= (gen >> 14) & pro;
    pro &= (pro >> 14);
    gen |= (gen >> 28) & pro;

    return gen;
}

uint64 BitBoardUtils::southWestFill(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen >> 9) & pro;
    pro &= (pro >> 9);
    gen |= (gen >> 18) & pro;
    pro &= (pro >> 18);
    gen |= (gen >> 36) & pro;

    return gen;
}


// attacks in the given direction
// need to OR with ~(pieces of side to move) to avoid killing own pieces

uint64 BitBoardUtils::northAttacks(uint64 gen, uint64 pro)
{
    gen |= (gen << 8) & pro;
    pro &= (pro << 8);
    gen |= (gen << 16) & pro;
    pro &= (pro << 16);
    gen |= (gen << 32) & pro;

    return gen << 8;
}

uint64 BitBoardUtils::southAttacks(uint64 gen, uint64 pro)
{
    gen |= (gen >> 8) & pro;
    pro &= (pro >> 8);
    gen |= (gen >> 16) & pro;
    pro &= (pro >> 16);
    gen |= (gen >> 32) & pro;

    return gen >> 8;
}

uint64 BitBoardUtils::eastAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen << 1) & pro;
    pro &= (pro << 1);
    gen |= (gen << 2) & pro;
    pro &= (pro << 2);
    gen |= (gen << 3) & pro;

    return (gen << 1) & (~FILEA);
}

uint64 BitBoardUtils::westAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen >> 1) & pro;
    pro &= (pro >> 1);
    gen |= (gen >> 2) & pro;
    pro &= (pro >> 2);
    gen |= (gen >> 3) & pro;

    return (gen >> 1) & (~FILEH);
}


uint64 BitBoardUtils::northEastAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen << 9) & pro;
    pro &= (pro << 9);
    gen |= (gen << 18) & pro;
    pro &= (pro << 18);
    gen |= (gen << 36) & pro;

    return (gen << 9) & (~FILEA);
}

uint64 BitBoardUtils::northWestAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen << 7) & pro;
    pro &= (pro << 7);
    gen |= (gen << 14) & pro;
    pro &= (pro << 14);
    gen |= (gen << 28) & pro;

    return (gen << 7) & (~FILEH);
}

uint64 BitBoardUtils::southEastAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEA;

    gen |= (gen >> 7) & pro;
    pro &= (pro >> 7);
    gen |= (gen >> 14) & pro;
    pro &= (pro >> 14);
    gen |= (gen >> 28) & pro;

    return (gen >> 7) & (~FILEA);
}

uint64 BitBoardUtils::southWestAttacks(uint64 gen, uint64 pro)
{
    pro &= ~FILEH;

    gen |= (gen >> 9) & pro;
    pro &= (pro >> 9);
    gen |= (gen >> 18) & pro;
    pro &= (pro >> 18);
    gen |= (gen >> 36) & pro;

    return (gen >> 9) & (~FILEH);
}


// attacks by pieces of given type
// pro - empty squares

uint64 BitBoardUtils::bishopAttacksKoggeStone(uint64 bishops, uint64 pro)
{
    return northEastAttacks(bishops, pro) |
           northWestAttacks(bishops, pro) |
           southEastAttacks(bishops, pro) |
           southWestAttacks(bishops, pro);
}

uint64 BitBoardUtils::rookAttacksKoggeStone(uint64 rooks, uint64 pro)
{
    return northAttacks(rooks, pro) |
           southAttacks(rooks, pro) |
           eastAttacks (rooks, pro) |
           westAttacks (rooks, pro);
}


#if USE_SLIDING_LUT == 1
uint64 BitBoardUtils::bishopAttacks(uint64 bishop, uint64 pro)
{
    uint8 square = bitScan(bishop);
    uint64 occ = (~pro) & sqBishopAttacksMasked(square);

#if USE_FANCY_MAGICS == 1
#ifdef __CUDA_ARCH__
    FancyMagicEntry magicEntry = sq_bishop_magics_fancy(square);
    int index = (magicEntry.factor * occ) >> (64 - BISHOP_MAGIC_BITS);

#if USE_BYTE_LOOKUP_FANCY == 1
    int index2 = sq_fancy_byte_magic_lookup_table(magicEntry.position + index) + magicEntry.offset;
    return sq_fancy_byte_BishopLookup(index2);
#else // USE_BYTE_LOOKUP_FANCY == 1
    return sq_fancy_magic_lookup_table(magicEntry.position + index);
#endif // USE_BYTE_LOOKUP_FANCY == 1

#else // #ifdef __CUDA_ARCH__
    // this version is slightly faster for CPUs.. why ?
    uint64 magic = bishop_magics_fancy[square].factor;
    uint64 index = (magic * occ) >> (64 - BISHOP_MAGIC_BITS);
#if USE_BYTE_LOOKUP_FANCY == 1
    uint8 *table = &fancy_byte_magic_lookup_table[bishop_magics_fancy[square].position];
    int index2 = table[index] + bishop_magics_fancy[square].offset;
    return fancy_byte_BishopLookup[index2];
#else // USE_BYTE_LOOKUP_FANCY == 1
    uint64 *table = &fancy_magic_lookup_table[bishop_magics_fancy[square].position];
    return table[index];
#endif // USE_BYTE_LOOKUP_FANCY == 1
#endif // #ifdef __CUDA_ARCH__
#else // USE_FANCY_MAGICS == 1
    uint64 magic = sqBishopMagics(square);
    uint64 index = (magic * occ) >> (64 - BISHOP_MAGIC_BITS);
    return sqBishopMagicAttackTables(square, index);
#endif // USE_FANCY_MAGICS == 1
}

uint64 BitBoardUtils::rookAttacks(uint64 rook, uint64 pro)
{
    uint8 square = bitScan(rook);
    uint64 occ = (~pro) & sqRookAttacksMasked(square);

#if USE_FANCY_MAGICS == 1
#ifdef __CUDA_ARCH__
    FancyMagicEntry magicEntry = sq_rook_magics_fancy(square);
    int index = (magicEntry.factor * occ) >> (64 - ROOK_MAGIC_BITS);
#if USE_BYTE_LOOKUP_FANCY == 1
    int index2 = sq_fancy_byte_magic_lookup_table(magicEntry.position + index) + magicEntry.offset;
    return sq_fancy_byte_RookLookup(index2);
#else
    return sq_fancy_magic_lookup_table(magicEntry.position + index);
#endif

#else
    // this version is slightly faster for CPUs.. why ?
    uint64 magic = rook_magics_fancy[square].factor;
    uint64 index = (magic * occ) >> (64 - ROOK_MAGIC_BITS);
#if USE_BYTE_LOOKUP_FANCY == 1
    uint8 *table = &fancy_byte_magic_lookup_table[rook_magics_fancy[square].position];
    int index2 = table[index] + rook_magics_fancy[square].offset;
    return fancy_byte_RookLookup[index2];
#else
    uint64 *table = &fancy_magic_lookup_table[rook_magics_fancy[square].position];
    return table[index];
#endif
#endif
#else
    uint64 magic = sqRookMagics(square);
    uint64 index = (magic * occ) >> (64 - ROOK_MAGIC_BITS);
    return sqRookMagicAttackTables(square, index);
#endif
}

uint64 BitBoardUtils::multiBishopAttacks(uint64 bishops, uint64 pro)
{
    uint64 attacks = 0;
    while (bishops)
    {
        uint64 bishop = getOne(bishops);
        attacks |= bishopAttacks(bishop, pro);
        bishops ^= bishop;
    }

    return attacks;
}

uint64 BitBoardUtils::multiRookAttacks(uint64 rooks, uint64 pro)
{
    uint64 attacks = 0;
    while (rooks)
    {
        uint64 rook = getOne(rooks);
        attacks |= rookAttacks(rook, pro);
        rooks ^= rook;
    }

    return attacks;
}
#endif

uint64 BitBoardUtils::multiKnightAttacks(uint64 knights)
{
    uint64 attacks = 0;
    while (knights)
    {
        uint64 knight = getOne(knights);
        attacks |= sqKnightAttacks(bitScan(knight));
        knights ^= knight;
    }
    return attacks;
}


// not used
#if 0
uint64 BitBoardUtils::queenAttacks(uint64 queens, uint64 pro)
{
    return rookAttacks(queens, pro) |
        bishopAttacks(queens, pro);
}
#endif

uint64 BitBoardUtils::kingAttacks(uint64 kingSet)
{
    uint64 attacks = eastOne(kingSet) | westOne(kingSet);
    kingSet |= attacks;
    attacks |= northOne(kingSet) | southOne(kingSet);
    return attacks;
}

// efficient knight attack generator
// http://chessprogramming.wikispaces.com/Knight+Pattern
uint64 BitBoardUtils::knightAttacks(uint64 knights) {
    uint64 l1 = (knights >> 1) & C64(0x7f7f7f7f7f7f7f7f);
    uint64 l2 = (knights >> 2) & C64(0x3f3f3f3f3f3f3f3f);
    uint64 r1 = (knights << 1) & C64(0xfefefefefefefefe);
    uint64 r2 = (knights << 2) & C64(0xfcfcfcfcfcfcfcfc);
    uint64 h1 = l1 | r1;
    uint64 h2 = l2 | r2;
    return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}

// finds the squares in between the two given squares
// taken from 
// http://chessprogramming.wikispaces.com/Square+Attacked+By#Legality Test-In Between-Pure Calculation
uint64 BitBoardUtils::squaresInBetween(uint8 sq1, uint8 sq2)
{
    const uint64 m1 = C64(0xFFFFFFFFFFFFFFFF);
    const uint64 a2a7 = C64(0x0001010101010100);
    const uint64 b2g7 = C64(0x0040201008040200);
    const uint64 h1b7 = C64(0x0002040810204080);
    uint64 btwn, line, rank, file;

    btwn = (m1 << sq1) ^ (m1 << sq2);
    file = (sq2 & 7) - (sq1 & 7);
    rank = ((sq2 | 7) - sq1) >> 3;
    line = ((file & 7) - 1) & a2a7; // a2a7 if same file
    line += 2 * (((rank & 7) - 1) >> 58); // b1g1 if same rank
    line += (((rank - file) & 15) - 1) & b2g7; // b2g7 if same diagonal
    line += (((rank + file) & 15) - 1) & h1b7; // h1b7 if same antidiag
    line *= btwn & -(static_cast<int64>(btwn)); // mul acts like shift by smaller square
    return line & btwn;   // return the bits on that line inbetween
}

// returns the 'line' containing all pieces in the same file/rank/diagonal or anti-diagonal containing sq1 and sq2
uint64 BitBoardUtils::squaresInLine(uint8 sq1, uint8 sq2)
{
    // TODO: try to make it branchless?
    int fileDiff = (sq2 & 7) - (sq1 & 7);
    int rankDiff = ((sq2 | 7) - sq1) >> 3;

    uint8 file = sq1 & 7;
    uint8 rank = sq1 >> 3;

    if (fileDiff == 0)  // same file
    {
        return FILEA << file;
    }
    if (rankDiff == 0)  // same rank
    {
        return RANK1 << (rank * 8);
    }
    if (fileDiff - rankDiff == 0)   // same diagonal (with slope equal to a1h8)
    {
        if (rank - file >= 0)
            return DIAGONAL_A1H8 << ((rank - file) * 8);
        else
            return DIAGONAL_A1H8 >> ((file - rank) * 8);
    }
    if (fileDiff + rankDiff == 0)  // same anti-diagonal (with slope equal to a8h1)
    {
        // for a8h1, rank + file = 7
        int shiftAmount = (rank + file - 7) * 8;
        if (shiftAmount >= 0)
            return DIAGONAL_A8H1 << shiftAmount;
        else
            return DIAGONAL_A8H1 >> (-shiftAmount);
    }

    // squares not on same line
    return 0;
}


uint64 BitBoardUtils::sqsInBetween(uint8 sq1, uint8 sq2)
{
#if USE_IN_BETWEEN_LUT == 1
    return sqsInBetweenLUT(sq1, sq2);
#else
    return squaresInBetween(sq1, sq2);
#endif
}

uint64 BitBoardUtils::sqsInLine(uint8 sq1, uint8 sq2)
{
#if USE_IN_BETWEEN_LUT == 1
    return sqsInLineLUT(sq1, sq2);
#else
    return squaresInLine(sq1, sq2);
#endif
}



void BitBoardUtils::updateCastleFlag(HexaBitBoardPosition *pos, uint64 dst, uint8 chance)
{

#if USE_BITWISE_MAGIC_FOR_CASTLE_FLAG_UPDATION == 1
    if (chance == WHITE)
    {
        pos->blackCastle &= ~(((dst & BLACK_KING_SIDE_ROOK) >> H8) |
            ((dst & BLACK_QUEEN_SIDE_ROOK) >> (A8 - 1)));
    }
    else
    {
        pos->whiteCastle &= ~(((dst & WHITE_KING_SIDE_ROOK) >> H1) |
            ((dst & WHITE_QUEEN_SIDE_ROOK) << 1));
    }
#else
    if (chance == WHITE)
    {
        if (dst & BLACK_KING_SIDE_ROOK)
            pos->blackCastle &= ~CASTLE_FLAG_KING_SIDE;
        else if (dst & BLACK_QUEEN_SIDE_ROOK)
            pos->blackCastle &= ~CASTLE_FLAG_QUEEN_SIDE;
    }
    else
    {
        if (dst & WHITE_KING_SIDE_ROOK)
            pos->whiteCastle &= ~CASTLE_FLAG_KING_SIDE;
        else if (dst & WHITE_QUEEN_SIDE_ROOK)
            pos->whiteCastle &= ~CASTLE_FLAG_QUEEN_SIDE;
    }
#endif
}



uint64 BitBoardUtils::findPinnedPieces(uint64 myKing, uint64 enemyBishops, uint64 enemyRooks, uint64 allPieces, uint8 kingIndex)
{
    // check for sliding attacks to the king's square

    // It doesn't matter if we process more attackers behind the first attackers
    // They will be taken care of when we check for no. of obstructing squares between king and the attacker
    /*
    uint64 b = bishopAttacks(myKing, ~enemyPieces) & enemyBishops;
    uint64 r = rookAttacks  (myKing, ~enemyPieces) & enemyRooks;
    */

    uint64 b = sqBishopAttacks(kingIndex) & enemyBishops;
    uint64 r = sqRookAttacks(kingIndex)   & enemyRooks;

    uint64 attackers = b | r;

    // for every attacker we need to chess if there is a single obstruction between 
    // the attacker and the king, and if so - the obstructor is pinned
    uint64 pinned = EMPTY;
    while (attackers)
    {
        uint64 attacker = getOne(attackers);

        // bitscan shouldn't be too expensive but it will be good to 
        // figure out a way do find obstructions without having to get square index of attacker
        uint8 attackerIndex = bitScan(attacker);    // same as bitscan on attackers

        uint64 squaresInBetween = sqsInBetween(attackerIndex, kingIndex); // same as using obstructed() function
        uint64 piecesInBetween = squaresInBetween & allPieces;
        if (isSingular(piecesInBetween))
            pinned |= piecesInBetween;

        attackers ^= attacker;  // same as &= ~attacker
    }

    return pinned;
}

// returns bitmask of squares in threat by enemy pieces
// the king shouldn't ever attempt to move to a threatened square
// TODO: maybe make this tempelated on color?
uint64 BitBoardUtils::findAttackedSquares(uint64 emptySquares, uint64 enemyBishops, uint64 enemyRooks,
                                          uint64 enemyPawns, uint64 enemyKnights, uint64 enemyKing,
                                          uint64 myKing, uint8 enemyColor)
{
    uint64 attacked = 0;

    // 1. pawn attacks
    if (enemyColor == WHITE)
    {
        attacked |= northEastOne(enemyPawns);
        attacked |= northWestOne(enemyPawns);
    }
    else
    {
        attacked |= southEastOne(enemyPawns);
        attacked |= southWestOne(enemyPawns);
    }

    // 2. knight attacks
#if USE_KNIGHT_LUT == 1
    attacked |= multiKnightAttacks(enemyKnights); // again a tiny bit faster
#else
    attacked |= knightAttacks(enemyKnights);
#endif

    // 3. bishop attacks
    attacked |= multiBishopAttacks(enemyBishops, emptySquares | myKing); // squares behind king are also under threat (in the sense that king can't go there)

    // 4. rook attacks
    attacked |= multiRookAttacks(enemyRooks, emptySquares | myKing); // squares behind king are also under threat

    // 5. King attacks
#if USE_KING_LUT == 1
    attacked |= sqKingAttacks(bitScan(enemyKing));	// a very tiny bit faster!
#else
    attacked |= kingAttacks(enemyKing);
#endif

    // TODO: 
    // 1. figure out if we really need to mask off pieces on board
    //  - actually it seems better not to.. so that we can easily check if a capture move takes the king to check
    // 2. It might be faster to use the lookup table instead of computing (esp for king and knights).. DONE!
    return attacked/*& (emptySquares)*/;
}



template <uint8 chance>
void BitBoardUtils::makeMove(HexaBitBoardPosition *pos, uint64 &hash, CMove move)
{
    uint64 src = BIT(move.getFrom());
    uint64 dst = BIT(move.getTo());

    // figure out the source piece
    uint64 queens = pos->bishopQueens & pos->rookQueens;
    uint8 piece = 0;
    if (pos->kings & src)
        piece = KING;
    else if (pos->knights & src)
        piece = KNIGHT;
    else if ((pos->pawns & RANKS2TO7) & src)
        piece = PAWN;
    else if (queens & src)
        piece = QUEEN;
    else if (pos->bishopQueens & src)
        piece = BISHOP;
    else
        piece = ROOK;

#if INCREMENTAL_ZOBRIST_UPDATE == 1
    // remove moving piece from source
    hash ^= zob.pieces[chance][piece - 1][move.getFrom()];
#endif

    // promote the pawn (if this was promotion move)
    if (move.getFlags() == CM_FLAG_KNIGHT_PROMOTION || move.getFlags() == CM_FLAG_KNIGHT_PROMO_CAP)
        piece = KNIGHT;
    else if (move.getFlags() == CM_FLAG_BISHOP_PROMOTION || move.getFlags() == CM_FLAG_BISHOP_PROMO_CAP)
        piece = BISHOP;
    else if (move.getFlags() == CM_FLAG_ROOK_PROMOTION || move.getFlags() == CM_FLAG_ROOK_PROMO_CAP)
        piece = ROOK;
    else if (move.getFlags() == CM_FLAG_QUEEN_PROMOTION || move.getFlags() == CM_FLAG_QUEEN_PROMO_CAP)
        piece = QUEEN;

#if INCREMENTAL_ZOBRIST_UPDATE == 1

    // remove captured piece from dst
    {
        uint8 dstPiece = 0;
        // figure out destination piece
        if (pos->kings & dst)
            dstPiece = KING;
        else if (pos->knights & dst)
            dstPiece = KNIGHT;
        else if ((pos->pawns & RANKS2TO7) & dst)
            dstPiece = PAWN;
        else if (queens & dst)
            dstPiece = QUEEN;
        else if (pos->bishopQueens & dst)
            dstPiece = BISHOP;
        else if (pos->rookQueens & dst)
            dstPiece = ROOK;

        if (dstPiece)
        {
            hash ^= zob.pieces[!chance][dstPiece - 1][move.getTo()];
        }
    }

    // add moving piece at dst
    hash ^= zob.pieces[chance][piece - 1][move.getTo()];

    // flip color
    hash ^= zob.chance;

    // clear special move flags
    // castling rights
    if (pos->whiteCastle & CASTLE_FLAG_KING_SIDE)
        hash ^= zob.castlingRights[WHITE][0];
    if (pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE)
        hash ^= zob.castlingRights[WHITE][1];

    if (pos->blackCastle & CASTLE_FLAG_KING_SIDE)
        hash ^= zob.castlingRights[BLACK][0];
    if (pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE)
        hash ^= zob.castlingRights[BLACK][1];


    // en-passent target
    if (pos->enPassent)
    {
        hash ^= zob.enPassentTarget[pos->enPassent - 1];
    }
#endif

    // remove source from all bitboards
    pos->bishopQueens &= ~src;
    pos->rookQueens &= ~src;
    pos->kings &= ~src;
    pos->knights &= ~src;
    pos->pawns &= ~(src & RANKS2TO7);

    // remove the dst from all bitboards
    pos->bishopQueens &= ~dst;
    pos->rookQueens &= ~dst;
    pos->kings &= ~dst;
    pos->knights &= ~dst;
    pos->pawns &= ~(dst & RANKS2TO7);

    // put the piece that moved in the required bitboards
    if (piece == KING)
    {
        pos->kings |= dst;

        if (chance == WHITE)
            pos->whiteCastle = 0;
        else
            pos->blackCastle = 0;
    }

    if (piece == KNIGHT)
        pos->knights |= dst;

    if (piece == PAWN)
        pos->pawns |= dst;

    if (piece == BISHOP || piece == QUEEN)
        pos->bishopQueens |= dst;

    if (piece == ROOK || piece == QUEEN)
        pos->rookQueens |= dst;


    if (chance == WHITE)
    {
        pos->whitePieces = (pos->whitePieces ^ src) | dst;
    }
    else
    {
        pos->whitePieces = pos->whitePieces  & ~dst;
    }

    // if it's an en-passet move, remove the captured pawn also
    if (move.getFlags() == CM_FLAG_EP_CAPTURE)
    {
        uint64 enPassentCapturedPiece = (chance == WHITE) ? southOne(dst) : northOne(dst);

        pos->pawns &= ~(enPassentCapturedPiece & RANKS2TO7);

#if INCREMENTAL_ZOBRIST_UPDATE == 1
        hash ^= zob.pieces[!chance][ZOB_INDEX_PAWN][bitScan(enPassentCapturedPiece)];
#endif
        if (chance == BLACK)
            pos->whitePieces &= ~enPassentCapturedPiece;
    }

    // if it's a castling, move the rook also
    if (chance == WHITE)
    {
        if (move.getFlags() == CM_FLAG_KING_CASTLE)
        {
            // white castle king side
            pos->rookQueens = (pos->rookQueens  ^ BIT(H1)) | BIT(F1);
            pos->whitePieces = (pos->whitePieces ^ BIT(H1)) | BIT(F1);
#if INCREMENTAL_ZOBRIST_UPDATE == 1
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][H1];
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][F1];
#endif
        }
        else if (move.getFlags() == CM_FLAG_QUEEN_CASTLE)
        {
            // white castle queen side
            pos->rookQueens = (pos->rookQueens  ^ BIT(A1)) | BIT(D1);
            pos->whitePieces = (pos->whitePieces ^ BIT(A1)) | BIT(D1);
#if INCREMENTAL_ZOBRIST_UPDATE == 1
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][A1];
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][D1];
#endif
        }
    }
    else
    {
        if (move.getFlags() == CM_FLAG_KING_CASTLE)
        {
            // black castle king side
            pos->rookQueens = (pos->rookQueens  ^ BIT(H8)) | BIT(F8);
#if INCREMENTAL_ZOBRIST_UPDATE == 1
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][H8];
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][F8];
#endif
        }
        else if (move.getFlags() == CM_FLAG_QUEEN_CASTLE)
        {
            // black castle queen side
            pos->rookQueens = (pos->rookQueens  ^ BIT(A8)) | BIT(D8);
#if INCREMENTAL_ZOBRIST_UPDATE == 1
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][A8];
            hash ^= zob.pieces[chance][ZOB_INDEX_ROOK][D8];
#endif
        }
    }


    // update the game state
    pos->chance = !chance;
    pos->enPassent = 0;
    //pos->halfMoveCounter++;   // quiet move -> increment half move counter // TODO: correctly increment this based on if there was a capture
    updateCastleFlag(pos, dst, chance);

    if (piece == ROOK)
    {
        updateCastleFlag(pos, src, !chance);
    }

    if (move.getFlags() == CM_FLAG_DOUBLE_PAWN_PUSH)
    {
        pos->enPassent = (move.getFrom() & 7) + 1;      // store file + 1
    }

#if INCREMENTAL_ZOBRIST_UPDATE == 1
    // add special move flags
    // castling rights
    if (pos->whiteCastle & CASTLE_FLAG_KING_SIDE)
        hash ^= zob.castlingRights[WHITE][0];
    if (pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE)
        hash ^= zob.castlingRights[WHITE][1];

    if (pos->blackCastle & CASTLE_FLAG_KING_SIDE)
        hash ^= zob.castlingRights[BLACK][0];
    if (pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE)
        hash ^= zob.castlingRights[BLACK][1];


    // en-passent target
    if (pos->enPassent)
    {
        hash ^= zob.enPassentTarget[pos->enPassent - 1];
    }
#endif

}



// compute zobrist hash key for a given board position
uint64 BitBoardUtils::ComputeZobristKey(HexaBitBoardPosition *pos)
{

    uint64 key = 0;
    int chance = pos->chance;

    // chance (side to move)
    if (chance)
        key ^= zob.chance;

    // castling rights
    if (pos->whiteCastle & CASTLE_FLAG_KING_SIDE)
        key ^= zob.castlingRights[WHITE][0];
    if (pos->whiteCastle & CASTLE_FLAG_QUEEN_SIDE)
        key ^= zob.castlingRights[WHITE][1];

    if (pos->blackCastle & CASTLE_FLAG_KING_SIDE)
        key ^= zob.castlingRights[BLACK][0];
    if (pos->blackCastle & CASTLE_FLAG_QUEEN_SIDE)
        key ^= zob.castlingRights[BLACK][1];



    uint64 allPawns = pos->pawns & RANKS2TO7;    // get rid of game state variables
    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;

    // en-passent target
    if (pos->enPassent)
    {
        uint64 blackPieces = allPieces & (~pos->whitePieces);
        uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
        uint64 myPawns = allPawns & myPieces;

        // only update the flag if en-passent is possible
        uint64 enPassentCapturedPiece;
        if (chance == BLACK)
        {
            enPassentCapturedPiece = BIT(pos->enPassent - 1) << (8 * 3);
        }
        else
        {
            enPassentCapturedPiece = BIT(pos->enPassent - 1) << (8 * 4);
        }
        uint64 epSources = (eastOne(enPassentCapturedPiece) | westOne(enPassentCapturedPiece)) & myPawns;

        if (epSources)
        {
            key ^= zob.enPassentTarget[pos->enPassent - 1];
        }
    }

    // piece-position
    while (allPieces)
    {
        uint64 piece = getOne(allPieces);
        int square   = bitScan(piece);

        int color = !(piece & pos->whitePieces);
        if (piece & allPawns)
        {
            key ^= zob.pieces[color][ZOB_INDEX_PAWN][square];
        }
        else if (piece & pos->kings)
        {
            key ^= zob.pieces[color][ZOB_INDEX_KING][square];
        }
        else if (piece & pos->knights)
        {
            key ^= zob.pieces[color][ZOB_INDEX_KNIGHT][square];
        }
        else if (piece & pos->rookQueens & pos->bishopQueens)
        {
            key ^= zob.pieces[color][ZOB_INDEX_QUEEN][square];
        }
        else if (piece & pos->rookQueens)
        {
            key ^= zob.pieces[color][ZOB_INDEX_ROOK][square];
        }
        else if (piece & pos->bishopQueens)
        {
            key ^= zob.pieces[color][ZOB_INDEX_BISHOP][square];
        }

        allPieces ^= piece;
    }


    return key;
}




//----------------------------------------- Public routines -------------------------------------------//



void BitBoardUtils::init()
{
    // initialize zobrist keys
    memcpy(&zob, &randoms[777], sizeof(zob));

    // initialize the empty board attack tables
    for (uint8 i = 0; i < 64; i++)
    {
        uint64 x = BIT(i);
        uint64 north = northAttacks(x, ALLSET);
        uint64 south = southAttacks(x, ALLSET);
        uint64 east = eastAttacks(x, ALLSET);
        uint64 west = westAttacks(x, ALLSET);
        uint64 ne = northEastAttacks(x, ALLSET);
        uint64 nw = northWestAttacks(x, ALLSET);
        uint64 se = southEastAttacks(x, ALLSET);
        uint64 sw = southWestAttacks(x, ALLSET);

        RookAttacks[i] = north | south | east | west;
        BishopAttacks[i] = ne | nw | se | sw;
        QueenAttacks[i] = RookAttacks[i] | BishopAttacks[i];
        KnightAttacks[i] = knightAttacks(x);
        KingAttacks[i] = kingAttacks(x);

        // TODO: initialize pawn attack table
        // probably not really needed (as pawn attack calculation is simple enough)
    }

    // initialize the Between and Line tables
    for (uint8 i = 0; i<64; i++)
        for (uint8 j = 0; j<64; j++)
        {
            if (i <= j)
            {
                Between[i][j] = squaresInBetween(i, j);
                Between[j][i] = Between[i][j];
            }
            Line[i][j] = squaresInLine(i, j);
        }


    // initialize magic lookup tables
#if USE_SLIDING_LUT == 1
    srand(time(NULL));
    for (int square = A1; square <= H8; square++)
    {
        uint64 thisSquare = BIT(square);
        uint64 mask = sqRookAttacks(square) & (~thisSquare);

        // mask off squares that don't matter
        if ((thisSquare & RANK1) == 0)
            mask &= ~RANK1;

        if ((thisSquare & RANK8) == 0)
            mask &= ~RANK8;

        if ((thisSquare & FILEA) == 0)
            mask &= ~FILEA;

        if ((thisSquare & FILEH) == 0)
            mask &= ~FILEH;

        RookAttacksMasked[square] = mask;

        mask = sqBishopAttacks(square)  & (~thisSquare) & CENTRAL_SQUARES;
        BishopAttacksMasked[square] = mask;
#if USE_FANCY_MAGICS != 1
        rookMagics[square]   = findRookMagicForSquare(square, rookMagicAttackTables[square]);
        bishopMagics[square] = findBishopMagicForSquare(square, bishopMagicAttackTables[square]);
#endif
    }

    // initialize fancy magic lookup table
    memset(fancy_magic_lookup_table, 0, sizeof(fancy_magic_lookup_table));
    int globalOffsetRook = 0;
    int globalOffsetBishop = 0;

    for (int square = A1; square <= H8; square++)
    {
        int uniqueBishopAttacks = 0, uniqueRookAttacks = 0;

        uint64 rookMagic = findRookMagicForSquare(square, &fancy_magic_lookup_table[rook_magics_fancy[square].position], rook_magics_fancy[square].factor,
                                                  &fancy_byte_RookLookup[globalOffsetRook], &fancy_byte_magic_lookup_table[rook_magics_fancy[square].position], &uniqueRookAttacks);
        assert(rookMagic == rook_magics_fancy[square].factor);

        uint64 bishopMagic = findBishopMagicForSquare(square, &fancy_magic_lookup_table[bishop_magics_fancy[square].position], bishop_magics_fancy[square].factor,
                                                      &fancy_byte_BishopLookup[globalOffsetBishop], &fancy_byte_magic_lookup_table[bishop_magics_fancy[square].position], &uniqueBishopAttacks);
        assert(bishopMagic == bishop_magics_fancy[square].factor);

        rook_magics_fancy[square].offset = globalOffsetRook;
        globalOffsetRook += uniqueRookAttacks;

        bishop_magics_fancy[square].offset = globalOffsetBishop;
        globalOffsetBishop += uniqueBishopAttacks;
    }

    //printf("\ntotal bishop unique attacks: %d\n", globalOffsetBishop);
    //printf("\ntotal rook unique attacks: %d\n", globalOffsetRook);
#endif        
}


// a wrapper over the templated function above
void BitBoardUtils::MakeMove(HexaBitBoardPosition *pos, uint64 &hash, CMove move)
{
    uint8 chance = pos->chance;
    if (chance == WHITE)
    {
        BitBoardUtils::makeMove<WHITE>(pos, hash, move);
    }
    else
    {
        assert(chance == BLACK);
        BitBoardUtils::makeMove<BLACK>(pos, hash, move);
    }
}

// Ankan - TODO: check if this can be optimized? Maybe simplify the bitboard structure?
bool BitBoardUtils::IsInCheck(HexaBitBoardPosition *pos)
{
    uint8 chance = pos->chance;
    uint64 allPawns = pos->pawns & RANKS2TO7;

    uint64 allPieces = pos->kings | allPawns | pos->knights | pos->bishopQueens | pos->rookQueens;
    uint64 blackPieces = allPieces & (~pos->whitePieces);

    uint64 myPieces = (chance == WHITE) ? pos->whitePieces : blackPieces;
    uint64 enemyPieces = (chance == WHITE) ? blackPieces : pos->whitePieces;

    uint64 enemyBishops = pos->bishopQueens & enemyPieces;
    uint64 enemyRooks = pos->rookQueens & enemyPieces;

    uint64 myKing = pos->kings & myPieces;

    uint64 threatened = findAttackedSquares(~allPieces, enemyBishops, enemyRooks, allPawns & enemyPieces,
                                            pos->knights & enemyPieces, pos->kings & enemyPieces, myKing, !chance);

    if (threatened & myKing)
    {
        return true;
    }
    return false;
}


template<uint8 chance>
#if _WIN32 || _WIN64
// WAR for a possible compiler bug with VS 2013!
__forceinline
#endif
ExpandedBitBoard BitBoardUtils::ExpandBitBoard(HexaBitBoardPosition *pos)
{
    ExpandedBitBoard op;

    op.pawns            = pos->pawns & RANKS2TO7;
    op.allPieces        = pos->kings | op.pawns | pos->knights | pos->bishopQueens | pos->rookQueens;

    uint64 blackPieces  = op.allPieces & (~pos->whitePieces);

    op.myPieces         = (chance == WHITE) ? pos->whitePieces : blackPieces;
    op.enemyPieces      = (chance == WHITE) ? blackPieces : pos->whitePieces;

    op.kings            = pos->kings;
    op.knights          = pos->knights;
    op.bishopQueens     = pos->bishopQueens;
    op.rookQueens       = pos->rookQueens;


    op.myPawns          = op.pawns        & op.myPieces;
    op.myKnights        = op.knights      & op.myPieces;
    op.myBishopQueens   = op.bishopQueens & op.myPieces;
    op.myRookQueens     = op.rookQueens   & op.myPieces;
    op.myKing           = op.kings        & op.myPieces;

    op.enemyPawns       = op.pawns        & op.enemyPieces;
    op.enemyKnights     = op.knights      & op.enemyPieces;
    op.enemyKing        = op.kings        & op.enemyPieces;
    op.enemyBishopQueens= op.bishopQueens & op.enemyPieces;
    op.enemyRookQueens  = op.rookQueens   & op.enemyPieces;

    op.myKingIndex = bitScan(op.myKing);


    op.threatened       = findAttackedSquares(~op.allPieces, op.enemyBishopQueens, op.enemyRookQueens, op.enemyPawns, op.enemyKnights, op.enemyKing, op.myKing, !chance);
    op.pinned           = findPinnedPieces(op.myKing, op.enemyBishopQueens, op.enemyRookQueens, op.allPieces, op.myKingIndex);

    op.enPassent        = pos->enPassent;
    op.whiteCastle      = pos->whiteCastle;
    op.blackCastle = pos->blackCastle;

    return op;
}

bool BitBoardUtils::IsIrReversibleMove(HexaBitBoardPosition *pos, CMove move)
{
    uint64 src = BIT(move.getFrom());
    uint64 dst = BIT(move.getTo());

    // figure out the source piece
    uint64 queens = pos->bishopQueens & pos->rookQueens;
    uint8 piece = 0;
    if (pos->kings & src)
        piece = KING;
    else if (pos->knights & src)
        piece = KNIGHT;
    else if ((pos->pawns & RANKS2TO7) & src)
        piece = PAWN;
    else if (queens & src)
        piece = QUEEN;
    else if (pos->bishopQueens & src)
        piece = BISHOP;
    else
        piece = ROOK;


    if (piece == PAWN)
    {
        return true;
    }

    uint8 dstPiece = 0;
    // figure out destination piece
    if (pos->kings & dst)
        dstPiece = KING;
    else if (pos->knights & dst)
        dstPiece = KNIGHT;
    else if ((pos->pawns & RANKS2TO7) & dst)
        dstPiece = PAWN;
    else if (queens & dst)
        dstPiece = QUEEN;
    else if (pos->bishopQueens & dst)
        dstPiece = BISHOP;
    else if (pos->rookQueens & dst)
        dstPiece = ROOK;

    if (dstPiece)
    {
        return true;
    }

    if (move.getFlags() == CM_FLAG_KING_CASTLE || move.getFlags() == CM_FLAG_QUEEN_CASTLE)
    {
        return true;
    }

    return false;
}

template ExpandedBitBoard BitBoardUtils::ExpandBitBoard<WHITE>(HexaBitBoardPosition *pos);
template ExpandedBitBoard BitBoardUtils::ExpandBitBoard<BLACK>(HexaBitBoardPosition *pos);

template void BitBoardUtils::makeMove<WHITE>(HexaBitBoardPosition *pos, uint64 &hash, CMove move);
template void BitBoardUtils::makeMove<BLACK>(HexaBitBoardPosition *pos, uint64 &hash, CMove move);



