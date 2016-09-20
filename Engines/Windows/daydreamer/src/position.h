
#ifndef POSITION_H
#define POSITION_H
#ifdef __cplusplus
extern "C" {
#endif

#define FEN_STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define HASH_HISTORY_LENGTH  2048

typedef struct {
    piece_t _board_storage[256];        // 16x16 padded board
    piece_t* board;                     // 0x88 board in middle 128 slots
    int piece_index[128];               // index of each piece in pieces
    square_t pieces[2][32];
    square_t pawns[2][16];
    int num_pieces[2];
    int num_pawns[2];
    int piece_count[16];
    color_t side_to_move;
    move_t prev_move;
    square_t ep_square;
    int fifty_move_counter;
    int ply;
    int material_eval[2];
    score_t piece_square_eval[2];
    castle_rights_t castle_rights;
    uint8_t is_check;
    square_t check_square;
    hashkey_t hash;
    hashkey_t pawn_hash;
    hashkey_t material_hash;
    hashkey_t hash_history[HASH_HISTORY_LENGTH];
} position_t;

typedef struct {
    uint8_t is_check;
    square_t check_square;
    move_t prev_move;
    square_t ep_square;
    int fifty_move_counter;
    castle_rights_t castle_rights;
    hashkey_t hash;
} undo_info_t;

#ifdef __cplusplus
} // extern "C"
#endif
#endif // POSITION_H

