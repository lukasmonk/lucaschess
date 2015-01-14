
#ifndef EVAL_H
#define EVAL_H
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PHASE           24

#define PAWN_VAL         85
#define KNIGHT_VAL       350
#define BISHOP_VAL       350
#define ROOK_VAL         500
#define QUEEN_VAL        1000
#define KING_VAL         20000
#define EG_PAWN_VAL      115
#define EG_KNIGHT_VAL    400
#define EG_BISHOP_VAL    400
#define EG_ROOK_VAL      650
#define EG_QUEEN_VAL     1200
#define EG_KING_VAL      20000
#define WON_ENDGAME     (2*EG_QUEEN_VAL)

extern int piece_square_values[BK+1][0x80];
extern int endgame_piece_square_values[BK+1][0x80];
extern const int material_values[];
extern const int eg_material_values[];
#define material_value(piece)               material_values[piece]
#define eg_material_value(piece)            eg_material_values[piece]
#define piece_square_value(piece, square)   piece_square_values[piece][square]
#define endgame_piece_square_value(piece, square) \
    endgame_piece_square_values[piece][square]

typedef struct {
    int midgame;
    int endgame;
} score_t;

#include "position.h"

typedef struct {
    bitboard_t pawns_bb[2];
    bitboard_t outposts_bb[2];
    bitboard_t passed_bb[2];
    square_t passed[2][8];
    int num_passed[2];
    score_t score[2];
    int kingside_storm[2];
    int queenside_storm[2];
    hashkey_t key;
} pawn_data_t;

#define square_is_outpost(pd, sq, side) \
    (sq_bit_is_set((pd)->outposts_bb[side], (sq)))
#define file_is_half_open(pd, file, side) \
    (((pd)->pawns_bb[side] & file_mask[file]) == EMPTY_BB)
#define pawn_is_passed(pd, sq, side) \
    (sq_bit_is_set((pd)->passed_bb[side], (sq)))

typedef enum {
    EG_NONE,
    EG_WIN,
    EG_DRAW,
    EG_KBNK,
    EG_KPK,
    EG_LAST
} endgame_type_t;

typedef struct {
    hashkey_t key;
    endgame_type_t eg_type;
    int phase;
    int scale[2];
    score_t score;
    color_t strong_side;
} material_data_t;

typedef struct {
    pawn_data_t* pd;
    material_data_t* md;
} eval_data_t;

typedef void(*eg_scale_fn)(const position_t*, eval_data_t*, int scale[2]);
typedef int(*eg_score_fn)(const position_t*, eval_data_t*);
#define endgame_scale_function(md)   (eg_scale_fns[(md)->eg_type])

#ifdef __cplusplus
} // extern "C"
#endif
#endif // EVAL_H
