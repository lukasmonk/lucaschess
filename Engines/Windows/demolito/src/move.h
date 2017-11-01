#pragma once
#include "bitboard.h"
#include "types.h"

class Position;

typedef uint16_t move_t;    // from:6, to:6, prom: 3 (NB_PIECE if none)

struct Move {
    Square from, to;
    Piece prom;

    bool ok() const;

    Move() = default;
    Move(move_t em) { *this = em; }
    Move(const Position& pos, const std::string& s) { from_string(pos, s); }

    operator move_t() const;
    Move operator =(move_t em);

    bool null() const { return (from | to | prom) == 0; }
    bool is_capture(const Position& pos) const;
    bool is_castling(const Position& pos) const;

    std::string to_string(const Position& pos) const;
    void from_string(const Position& pos, const std::string& s);

    bool pseudo_is_legal(const Position& pos) const;
    int see(const Position& pos) const;
};
