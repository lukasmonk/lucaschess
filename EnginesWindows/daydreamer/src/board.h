
#ifndef BOARD_H
#define BOARD_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Definitions for colors, pieces, squares, directions, etc.
 */

typedef enum {
    WHITE=0, BLACK=1, INVALID_COLOR=2
} color_t;

typedef enum {
    EMPTY=0, WP=1, WN=2, WB=3, WR=4, WQ=5, WK=6,
    BP=9, BN=10, BB=11, BR=12, BQ=13, BK=14,
    OUT_OF_BOUNDS=16
} piece_t;

typedef enum {
    NONE=0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
} piece_type_t;

#define piece_type(piece)               ((piece) & 0x07)
#define piece_is_type(piece, type)      (piece_type((piece)) == (type))
#define piece_color(piece)              ((piece) >> 3)
#define piece_is_color(piece, color)    (piece_color((piece)) == (color))
#define create_piece(color, type)       (((color) << 3) | (type))
#define piece_colors_match(p1, p2)      (((p1) >> 3) == ((p2) >> 3))
#define piece_colors_differ(p1, p2)     (((p1) >> 3) != ((p2) >> 3))
#define can_capture(p1, p2)             ((((p1) >> 3)^1) == ((p2) >> 3))
#define flip_piece(p)                   (flip_piece[p])

typedef enum {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE
} file_tag_t;
typedef int file_t;

typedef enum {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE
} rank_tag_t;
typedef int rank_t;

typedef enum {
    A1=0x00, B1=0x01, C1=0x02, D1=0x03, E1=0x04, F1=0x05, G1=0x06, H1=0x07,
    A2=0x10, B2=0x11, C2=0x12, D2=0x13, E2=0x14, F2=0x15, G2=0x16, H2=0x17,
    A3=0x20, B3=0x21, C3=0x22, D3=0x23, E3=0x24, F3=0x25, G3=0x26, H3=0x27,
    A4=0x30, B4=0x31, C4=0x32, D4=0x33, E4=0x34, F4=0x35, G4=0x36, H4=0x37,
    A5=0x40, B5=0x41, C5=0x42, D5=0x43, E5=0x44, F5=0x45, G5=0x46, H5=0x47,
    A6=0x50, B6=0x51, C6=0x52, D6=0x53, E6=0x54, F6=0x55, G6=0x56, H6=0x57,
    A7=0x60, B7=0x61, C7=0x62, D7=0x63, E7=0x64, F7=0x65, G7=0x66, H7=0x67,
    A8=0x70, B8=0x71, C8=0x72, D8=0x73, E8=0x74, F8=0x75, G8=0x76, H8=0x77,
    INVALID_SQUARE=0x4b // just some square from the middle of the invalid part
} square_tag_t;
typedef int square_t;

#define square_rank(square)         ((square) >> 4)
#define square_file(square)         ((square) & 0x0f)
#define square_color(square)        (((square) ^ ((square) >> 4) ^ 1) & 1)
#define create_square(file, rank)   (((rank) << 4) | (file))
#define valid_board_index(idx)      !((idx) & 0x88)
#define flip_square(square)         ((square) ^ 0x70)
#define mirror_rank(square)         ((square) ^ 0x70)
#define mirror_file(square)         ((square) ^ 0x07)
#define square_to_index(square)     ((square)+((square) & 0x07))>>1
#define index_to_square(index)      ((index)+((index) & ~0x07))

typedef uint8_t castle_rights_t;
#define WHITE_OO                        0x01
#define BLACK_OO                        0x01 << 1
#define WHITE_OOO                       0x01 << 2
#define BLACK_OOO                       0x01 << 3
#define CASTLE_ALL                      (WHITE_OO | BLACK_OO | \
                                            WHITE_OOO | BLACK_OOO)
#define CASTLE_NONE                     0
#define has_oo_rights(pos, side)        ((pos)->castle_rights & \
                                            (WHITE_OO<<(side)))
#define has_ooo_rights(pos, side)       ((pos)->castle_rights & \
                                            (WHITE_OOO<<(side)))
#define can_castle(pos, side)           (has_oo_rights(pos,side) || \
                                            has_ooo_rights(pos,side))
#define add_oo_rights(pos, side)        ((pos)->castle_rights |= \
                                            (WHITE_OO<<(side)))
#define add_ooo_rights(pos, side)       ((pos)->castle_rights |= \
                                            (WHITE_OOO<<(side)))
#define remove_oo_rights(pos, side)     ((pos)->castle_rights &= \
                                            ~(WHITE_OO<<(side)))
#define remove_ooo_rights(pos, side)    ((pos)->castle_rights &= \
                                            ~(WHITE_OOO<<(side)))

typedef enum {
    SSW=-33, SSE=-31,
    WSW=-18, SW=-17, S=-16, SE=-15, ESE=-14,
    W=-1, STATIONARY=0, E=1,
    WNW=14, NW=15, N=16, NE=17, ENE=18,
    NNW=31, NNE=33
} direction_tag_t;
typedef int direction_t;
extern const direction_t piece_deltas[17][16];

extern const direction_t pawn_push[];
extern const rank_t relative_rank[2][8];
extern const piece_t flip_piece[16];

extern square_t king_home;
extern square_t king_rook_home;
extern square_t queen_rook_home;

#ifdef __cplusplus
} // extern "C"
#endif
#endif // BOARD_H

