
#include "daydreamer.h"

bitboard_t rank_mask[8] = {
    RANK_1_BB, RANK_2_BB, RANK_3_BB, RANK_4_BB,
    RANK_5_BB, RANK_6_BB, RANK_7_BB, RANK_8_BB
};
bitboard_t file_mask[8] = {
    FILE_A_BB, FILE_B_BB, FILE_C_BB, FILE_D_BB,
    FILE_E_BB, FILE_F_BB, FILE_G_BB, FILE_H_BB
};
bitboard_t neighbor_file_mask[8] = {
    FILE_B_BB, FILE_A_BB|FILE_C_BB, FILE_B_BB|FILE_D_BB, FILE_C_BB|FILE_E_BB,
    FILE_D_BB|FILE_F_BB, FILE_E_BB|FILE_G_BB, FILE_F_BB|FILE_H_BB, FILE_G_BB
};
bitboard_t set_mask[64];
bitboard_t clear_mask[64];
bitboard_t in_front_mask[2][64];
bitboard_t outpost_mask[2][64];
bitboard_t passed_mask[2][64];
const int bit_table[64] = {
     0,  1,  2,  7,  3, 13,  8, 19,
     4, 25, 14, 28,  9, 34, 20, 40,
     5, 17, 26, 38, 15, 46, 29, 48,
    10, 31, 35, 54, 21, 50, 41, 57,
    63,  6, 12, 18, 24, 27, 33, 39,
    16, 37, 45, 47, 30, 53, 49, 56,
    62, 11, 23, 32, 36, 44, 52, 55,
    61, 22, 43, 51, 60, 42, 59, 58
};

/*
 * Set all static bitboards to their appropriate values.
 */
void init_bitboards(void)
{
    for (int sq=0; sq<64; ++sq) {
        set_mask[sq] = 1ull<<sq;
        clear_mask[sq] = ~set_mask[sq];
        int sq_rank = sq / 8;
        int sq_file = sq % 8;
        in_front_mask[WHITE][sq] = in_front_mask[BLACK][sq] = EMPTY_BB;
        outpost_mask[WHITE][sq] = outpost_mask[BLACK][sq] = EMPTY_BB;
        passed_mask[WHITE][sq] = passed_mask[BLACK][sq] = EMPTY_BB;
        for (int rank = sq_rank+1; rank<8; ++rank) {
            outpost_mask[WHITE][sq] |= rank_mask[rank];
            passed_mask[WHITE][sq] |= rank_mask[rank];
            in_front_mask[WHITE][sq] |= rank_mask[rank];
        }
        for (int rank = sq_rank-1; rank>=0; --rank) {
            outpost_mask[BLACK][sq] |= rank_mask[rank];
            passed_mask[BLACK][sq] |= rank_mask[rank];
            in_front_mask[BLACK][sq] |= rank_mask[rank];
        }
        bitboard_t passer_file_mask =
            file_mask[sq_file] | neighbor_file_mask[sq_file];
        bitboard_t outpost_file_mask = neighbor_file_mask[sq_file];
        outpost_mask[WHITE][sq] &= outpost_file_mask;
        outpost_mask[BLACK][sq] &= outpost_file_mask;
        passed_mask[WHITE][sq] &= passer_file_mask;
        passed_mask[BLACK][sq] &= passer_file_mask;
        in_front_mask[WHITE][sq] &= file_mask[sq_file];
        in_front_mask[BLACK][sq] &= file_mask[sq_file];
    }
}

/*
 * Print an ascii representation of the given bitboard. Useful for debugging.
 */
void print_bitboard(bitboard_t bb)
{
    printf("%llx\n", bb);
    for (int rank=7; rank>=0; --rank) {
        char ch;
        for (int file=0; file<8; ++file) {
            ch = '.';
            bitboard_t bit = 1ull<<(rank*8+file);
            if (bb & bit) ch = '*';
            printf("%c ", ch);
        }
        printf("\n");
    }
    printf("\n");
}
