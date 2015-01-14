
#ifndef HASH_H
#define HASH_H
#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t hashkey_t;

extern const hashkey_t piece_random[2][7][64];
extern const hashkey_t castle_random[2][2][2];
extern const hashkey_t enpassant_random[64];

#define piece_hash(p,sq) \
    piece_random[piece_color(p)][piece_type(p)][square_to_index(sq)]
#define ep_hash(pos) \
    enpassant_random[square_to_index((pos)->ep_square)]
#define castle_hash(pos) \
    (castle_random[has_oo_rights(pos, WHITE) ? 1 : 0][0][0] ^ \
    castle_random[has_ooo_rights(pos, WHITE) ? 1 : 0][0][1] ^ \
    castle_random[has_oo_rights(pos, BLACK) ? 1 : 0][1][0] ^ \
    castle_random[has_ooo_rights(pos, BLACK) ? 1 : 0][1][1])
#define side_hash(pos)  ((pos)->side_to_move * 0x823a67c5f88337e7ull)
#define material_hash(p,count) \
    piece_random[piece_color(p)][piece_type(p)][count]

#ifdef __cplusplus
} // extern "C"
#endif
#endif // HASH_H
