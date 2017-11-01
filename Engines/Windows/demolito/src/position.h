#pragma once
#include "types.h"
#include "move.h"

class Position {
    bitboard_t _byColor[NB_COLOR];
    bitboard_t _byPiece[NB_PIECE];
    bitboard_t _castlableRooks;
    bitboard_t _attacked, _checkers, _pins;
    uint64_t _key, _pawnKey;
    eval_t _pst;
    char _pieceOn[NB_SQUARE];
    Color _turn;
    Square _epSquare;
    int _rule50;
    eval_t _pieceMaterial[NB_COLOR];

    void clear();
    void clear(Color c, Piece p, Square s);
    void set(Color c, Piece p, Square s);
    void finish();

public:
    void set(const std::string& fen);
    void set(const Position& before, Move m);
    void toggle(const Position& before);

    bitboard_t by_color(Color c) const;
    bitboard_t by_piece(Piece p) const;
    Color turn() const;
    Square ep_square() const;
    int rule50() const;
    bitboard_t checkers() const;
    bitboard_t attacked() const;
    bitboard_t pins() const;
    bitboard_t castlable_rooks() const;
    uint64_t key() const;
    uint64_t pawn_key() const;
    eval_t pst() const;
    eval_t piece_material(Color c) const;
    Piece piece_on(Square s) const;
};

bitboard_t attacked_by(const Position& pos, Color c);
bitboard_t calc_pins(const Position& pos);

uint64_t calc_key(const Position& pos);
uint64_t calc_pawn_key(const Position& pos);
eval_t calc_pst(const Position& pos);
eval_t calc_piece_material(const Position& pos, Color c);

bitboard_t pieces(const Position& pos);
bitboard_t pieces(const Position& pos, Piece p1, Piece p2);
bitboard_t pieces(const Position& pos, Color c, Piece p);
bitboard_t pieces(const Position& pos, Color c, Piece p1, Piece p2);

std::string get(const Position& pos);
bitboard_t ep_square_bb(const Position& pos);
bool insufficient_material(const Position& pos);
Square king_square(const Position& pos, Color c);
Color color_on(const Position& pos, Square s);
bitboard_t attackers_to(const Position& pos, Square s, bitboard_t occ);
void print(const Position& pos);
