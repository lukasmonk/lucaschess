
#ifndef MOVE_H
#define MOVE_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Each move is a 4-byte quantity that encodes source and destination
 * square, the moved piece, any captured piece, the promotion value (if any),
 * and flags to indicate en-passant capture and castling. The bit layout of
 * a move is as follows:
 *
 * 12345678  12345678  1234     5678     1234          5    6       78
 * FROM      TO        PIECE    CAPTURE  PROMOTE       EP   CASTLE  UNUSED
 * square_t  square_t  piece_t  piece_t  piece_type_t  bit  bit
 */
typedef int32_t move_t;
#define NO_MOVE                     0
#define NULL_MOVE                   0xffff
#define ENPASSANT_FLAG              1<<29
#define CASTLE_FLAG                 1<<30
#define get_move_from(move)         ((move) & 0xff)
#define get_move_to(move)           (((move) >> 8) & 0xff)
#define get_move_piece(move)        (((move) >> 16) & 0x0f)
#define get_move_piece_type(move) \
    ((piece_type_t)piece_type(get_move_piece(move)))
#define get_move_piece_color(move)  ((color_t)piece_color(get_move_piece(move)))
#define get_move_capture(move)      (((move) >> 20) & 0x0f)
#define get_move_capture_type(move) piece_type(get_move_capture(move))
#define get_move_promote(move)      (((move) >> 24) & 0x0f)
#define is_move_enpassant(move)     (((move) & ENPASSANT_FLAG) != 0)
#define is_move_castle(move)        (((move) & CASTLE_FLAG) != 0)
#define is_move_castle_long(move) \
    (is_move_castle(move) && (square_file(get_move_to(move)) == FILE_C))
#define is_move_castle_short(move) \
    (is_move_castle(move) && (square_file(get_move_to(move)) == FILE_G))
#define create_move(from, to, piece, capture) \
    ((from) | ((to) << 8) | ((piece) << 16) | ((capture) << 20))
#define create_move_promote(from, to, piece, capture, promote) \
    ((from) | ((to) << 8) | ((piece) << 16) | \
     ((capture) << 20) | ((promote) << 24))
#define create_move_castle(from, to, piece) \
    ((from) | ((to) << 8) | ((piece) << 16) | CASTLE_FLAG)
#define create_move_enpassant(from, to, piece, capture) \
    ((from) | ((to) << 8) | ((piece) << 16) | \
     ((capture) << 20) | ENPASSANT_FLAG)

typedef enum {
    NO_SLIDE=0, DIAGONAL, STRAIGHT, BOTH
} slide_t;

extern const slide_t sliding_piece_types[];
#define piece_slide_type(piece)     (sliding_piece_types[piece])

#ifdef __cplusplus
} // extern "C"
#endif
#endif // MOVE_H
