#pragma once
#include <cstdint>
#include <cassert>
#include <string>
#include <chrono>

extern bool Chess960;

extern int64_t dbgCnt[2];

#define BOUNDS(v, ub) assert(unsigned(v) < ub)

#define ENABLE_OPERATORS(T) \
inline constexpr T operator+(T v, int i) { return T(int(v) + i); } \
inline constexpr T operator-(T v, int i) { return T(int(v) - i); } \
inline T operator+=(T& v, int i) { return v = T(int(v) + i); } \
inline T operator-=(T& v, int i) { return v = T(int(v) - i); } \
inline T operator++(T& v) { return v = T(int(v) + 1); } \
inline T operator--(T& v) { return v = T(int(v) - 1); }

/* Color, Piece */

enum Color {WHITE, BLACK, NB_COLOR};
enum Piece {KNIGHT, BISHOP, ROOK, QUEEN, KING, PAWN, NB_PIECE};

ENABLE_OPERATORS(Color)
ENABLE_OPERATORS(Piece)

inline Color operator~(Color c) { return Color(c ^ BLACK); }

/* Rank, File, Square */

enum Rank {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NB_RANK};
enum File {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NB_FILE};
enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NB_SQUARE
};

ENABLE_OPERATORS(Rank)
ENABLE_OPERATORS(File)
ENABLE_OPERATORS(Square)

Rank rank_of(Square s);
File file_of(Square s);
Rank relative_rank(Color c, Rank r);
Rank relative_rank(Color c, Square s);
Square square(Rank r, File f);

std::string square_to_string(Square s);
Square string_to_square(const std::string& s);

/* Directions */

enum {UP = 8, DOWN = -8, LEFT = -1, RIGHT = 1};

int push_inc(Color c);    // pawn push increment

/* Material values */

enum {
    OP = 158, EP = 200, P = (OP+EP)/2,
    N = 640, B = 640,
    R = 1046, Q = 1980
};

/* Eval */

enum {OPENING, ENDGAME, NB_PHASE};

/*#ifdef __clang__
typedef int eval_t __attribute__ (( ext_vector_type(2) ));
#else
typedef int eval_t __attribute__ (( vector_size(8) ));
#endif*/

struct eval_t {
    int v[NB_PHASE];

    int operator[](int phase) const { return v[phase]; }
    int op() const { return v[OPENING]; }
    int eg() const { return v[ENDGAME]; }
    int& op() { return v[OPENING]; }
    int& eg() { return v[ENDGAME]; }

    operator bool() const { return op() || eg(); }
    bool operator==(eval_t e) const { return op() == e.op() && eg() == e.eg(); }
    bool operator!=(eval_t e) const { return !(*this == e); }

    eval_t operator+(eval_t e) const { return {op() + e.op(), eg() + e.eg()}; }
    eval_t operator-(eval_t e) const { return {op() - e.op(), eg() - e.eg()}; }
    eval_t operator*(int x) const { return {op() * x, eg() * x}; }
    eval_t operator/(int x) const { return {op() / x, eg() / x}; }

    eval_t& operator+=(eval_t e) { return op() += e.op(), eg() += e.eg(), *this; }
    eval_t& operator-=(eval_t e) { return op() -= e.op(), eg() -= e.eg(), *this; }
    eval_t& operator*=(int x) { return op() *= x, eg() *= x, *this; }
    eval_t& operator/=(int x) { return op() /= x, eg() /= x, *this; }
};

extern const eval_t Material[NB_PIECE];

#define INF    32767
#define MATE    32000
#define MAX_DEPTH    127
#define MIN_DEPTH    -8
#define MAX_PLY        (MAX_DEPTH - MIN_DEPTH + 2)
#define MAX_GAME_PLY    1024

/* Clock */

class Clock {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
public:
    void reset();
    std::chrono::milliseconds::rep elapsed();
};

bool score_ok(int score);
bool is_mate_score(int score);
int mated_in(int ply);
int mate_in(int ply);

/* Display */

extern const std::string PieceLabel[NB_COLOR];
