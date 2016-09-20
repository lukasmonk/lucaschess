
#ifndef BITBOARD_H
#define BITBOARD_H
#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bitboard_t;

#define BIT             0x1ull
#define EMPTY_BB        0x0000000000000000ull
#define FULL_BB         0xFFFFFFFFFFFFFFFFull

#define FILE_A_BB       0x0101010101010101ull
#define FILE_B_BB       0x0202020202020202ull
#define FILE_C_BB       0x0404040404040404ull
#define FILE_D_BB       0x0808080808080808ull
#define FILE_E_BB       0x1010101010101010ull
#define FILE_F_BB       0x2020202020202020ull
#define FILE_G_BB       0x4040404040404040ull
#define FILE_H_BB       0x8080808080808080ull

#define RANK_1_BB       0x00000000000000FFull
#define RANK_2_BB       0x000000000000FF00ull
#define RANK_3_BB       0x0000000000FF0000ull
#define RANK_4_BB       0x00000000FF000000ull
#define RANK_5_BB       0x000000FF00000000ull
#define RANK_6_BB       0x0000FF0000000000ull
#define RANK_7_BB       0x00FF000000000000ull
#define RANK_8_BB       0xFF00000000000000ull

extern bitboard_t set_mask[64];
extern bitboard_t clear_mask[64];
extern bitboard_t rank_mask[8];
extern bitboard_t file_mask[8];
extern bitboard_t neighbor_file_mask[8];
extern bitboard_t in_front_mask[2][64];
extern bitboard_t outpost_mask[2][64];
extern bitboard_t passed_mask[2][64];
extern const int bit_table[64];

#define set_bit(bb, ind)        ((bb) |= set_mask[ind])
#define set_sq_bit(bb, sq)      ((bb) |= set_mask[square_to_index(sq)])
#define clear_bit(bb, ind)      ((bb) &= clear_mask[ind])
#define clear_sq_bit(bb, sq)    ((bb) &= clear_mask[square_to_index(sq)])
#define bit_is_set(bb, ind)     ((bb) & set_mask[ind])
#define sq_bit_is_set(bb, sq)   ((bb) & set_mask[square_to_index(sq)])
#define first_bit(bb)           \
    (bit_table[(((bb) & (~(bb)+1)) * 0x0218A392CD3D5DBFull) >> 58])

#ifdef __cplusplus
}
#endif
#endif
