
#include "daydreamer.h"
#include <string.h>

const hashkey_t piece_random[2][7][64];
const hashkey_t castle_random[2][2][2];
const hashkey_t enpassant_random[64];

/*
 * Get a 64 bit random key, used for hashing. Unfortunately this has to
 * work around some platform inconsistencies.
 */
static hashkey_t random_hashkey(void)
{
    // Sadly we only get 16 usable bits on Windows, so lots of stitching
    // is needed. This isn't performance-sensitive or cryptographical
    // though, so this should be fine.
    hashkey_t hash = random_32();
    hash <<= 32;
    hash |= random_32();
    return hash;
}

/*
 * Initialize the Zobrist hash tables. Only done once, at startup.
 */
void init_hash(void)
{
    int i;
    srandom_32(1);
    hashkey_t* _piece_random = (hashkey_t*)&piece_random[0][0][0];
    hashkey_t* _castle_random = (hashkey_t*)&castle_random[0][0][0];
    hashkey_t* _enpassant_random = (hashkey_t*)&enpassant_random[0];

    for (i=0; i<2*7*64; ++i) _piece_random[i] = random_hashkey();
    for (i=0; i<64; ++i) _enpassant_random[i] = random_hashkey();
    for (i=0; i<2*2*2; ++i) _castle_random[i] = random_hashkey();
}

/*
 * Calculate the hash of a position from scratch. Used to in |set_position|,
 * and to verify the correctness of incremental hash updates in debug mode.
 */
hashkey_t hash_position(const position_t* pos)
{
    hashkey_t hash = 0;
    for (square_t sq=A1; sq<=H8; ++sq) {
        if (!valid_board_index(sq) || !pos->board[sq]) continue;
        hash ^= piece_hash(pos->board[sq], sq);
    }
    hash ^= ep_hash(pos);
    hash ^= castle_hash(pos);
    hash ^= side_hash(pos);
    return hash;
}

/*
 * Calculate the pawn hash of a position from scratch.
 */
hashkey_t hash_pawns(const position_t* pos)
{
    hashkey_t hash = 0;
    for (square_t sq=A1; sq<=H8; ++sq) {
        if (!valid_board_index(sq) || !pos->board[sq]) continue;
        if (!piece_is_type(pos->board[sq], PAWN)) continue;
        hash ^= piece_hash(pos->board[sq], sq);
    }
    return hash;
}

/*
 * Calculate the material hash of a position from scratch. The material hash
 * is computed by xor-ing together n 64-bit keys for each color/piece
 * combination, where n is the number of those pieces on the board.
 */
hashkey_t hash_material(const position_t* pos)
{
    hashkey_t hash = 0;
    piece_t p = 0;
    for (piece_type_t pt = PAWN; pt<=KING; ++pt) {
        p = create_piece(WHITE, pt);
        for (int i=0; i<pos->piece_count[p]; ++i) hash ^= material_hash(p, i);
        p = create_piece(BLACK, pt);
        for (int i=0; i<pos->piece_count[p]; ++i) hash ^= material_hash(p, i);
    }
    return hash;
}

/*
 * Set all hashes associated with a position to their correct values.
 */
void set_hash(position_t* pos)
{
    pos->hash = hash_position(pos);
    pos->material_hash = hash_material(pos);
    pos->pawn_hash = hash_pawns(pos);
}
