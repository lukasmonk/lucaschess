/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef POSITION_H
#define POSITION_H

#include <iostream>
#include <string>

#include "types.h"
#include "movelist.h"

class Thread;
class PawnHash;
class MaterialHash;

class PositionHistory{
    public:
        Key key;            /**< Position Zobrist key. */
        Key pawn_key;
        Key mat_key;
        Piece capture;      /**< Captured piece. */
        int fifty;          /**< Fifty move rule */
        Square ep;          /**< En passant square. */
        Castle castle;      /**< Castle status. */
        Bitboard checkers;  /** Check pieces. */
        Move move;          /**< Played move. */
        int pvt[2];
};

class EvalInfo{
    public:
        int king_safety[2];
        int major_material[2];
        int eval[2];
        int att_num[2];
        int att_total[2];
        int mul[2];
};

class Position{
    public:
        Position();
        void update_current();
        void set_thread(Thread* thread);
        void add(const Piece p, const Square sq);
        void remove(const Square sq);
        void empty();
        void start();
        bool is_ok() const;
        void print() const;
        void move_do(const Move move);
        void move_do(const std::string move);
        void move_undo();
        bool move_legal(const Move move) const;
        bool move_gen_legal(const Move move) const;
        void set_fen(const std::string fen);
        int eval() const;
        void gen_capture(MoveList& ml) const;
        void gen_quiet(MoveList& ml) const;
		void gen_quiet(MoveList& ml, const int16_t history[14][64][64]) const;
		void gen_evasions(MoveList & ml, const int16_t history[14][64][64]) const;
        void gen_quiet_checks(MoveList & ml) const;
        inline bool is_in_check() const;
        inline Piece get_piece(const Square sq) const;
        int see(const Move move) const;
        void strip();
        bool draw() const;
        inline Key get_key() const;
        inline Key get_mat_key() const;
        inline Key get_pawn_key() const;
        inline int get_stm() const;
		inline Square get_ep() const;
		inline Piece get_captured() const;
        bool is_move_check(const Move move) const;
        bool is_capture_dangerous(const Move move) const;
        bool is_dangerous(const Move move) const;
        int move_value(const Move move) const;
        uint64_t get_book_key() const;
        Thread* thread;
        void egbb_info(int king[2], int piece[3], int square[3]) const;
        inline int num_pieces() const;
        inline bool egbb() const;
		inline bool doNullMove() const;
        std::string move_to_san(const Move move) const;
		bool is_passed(const Square sq) const;
		bool endgame() const;
    private:
        Bitboard bitboard[14];	/**< Bitboard presentation of pieces. */
        int num[14];            /**< Number of pieces */
        Bitboard occupied;      /**< Occupied squares */
        Piece piece[64];        /**< Piece types on a board squares. */
        Color stm;              /**< Side to move. */
        int ply;
        PositionHistory ph[200];
        PositionHistory * current;
        MaterialHash* material_hash;
        PawnHash* pawn_hash;

        static bool initialized;

        static Key SqKey[14][64];   /**< Zobrist square keys. */
        static Key MatKey[14][17];  /**< Zobrist material keys. */
        static Key EpKey[64];       /**< Zobrist en paasant keys. */
        static Key CastleKey[13];   /**< Zobrist castle keys. */
        static Key StmKey;          /**< Zobrist side to move key. */

        static Bitboard PawnAttack[2][64];      /**< Pawn attacks. */
        static Bitboard KnightAttack[64];       /**< Knight attacks. */
        static Bitboard KingAttack[64];         /**< King attacks. */
        static Bitboard KingAttack2[64];
        static Bitboard BishopAttack[64];
        static Bitboard RookAttack[64];
        static Bitboard QueenAttack[64];

        static const int PawnFrom[2][4];
        static int Direction[64][64];

        static const int magic_bb_r_shift[64];
        static const Bitboard magic_bb_r_magics[64];
        static const Bitboard magic_bb_r_mask[64];
        static const Bitboard* magic_bb_r_indices[64];
        static Bitboard magic_bb_r_db[102400];
        static const int magic_bb_b_shift[64];
        static const Bitboard magic_bb_b_magics[64];
        static const Bitboard magic_bb_b_mask[64];
        static const Bitboard* magic_bb_b_indices[64];
        static Bitboard magic_bb_b_db[5248];
        static const uint64_t book_key[781];

        static int PVT[14][64][2];

        inline Bitboard knight_attack(const Square sq) const;
        inline static Bitboard knight_attack(const Square sq, const Bitboard bb);
        inline Bitboard bishop_attack(const Square sq) const;
        inline static Bitboard bishop_attack(const Square sq, const Bitboard bb);
        inline Bitboard rook_attack(const Square sq) const;
        inline static Bitboard rook_attack(const Square sq, const Bitboard bb);
        inline Bitboard queen_attack(const Square sq) const;
        inline static Bitboard queen_attack(const Square sq, const Bitboard bb);
        inline Bitboard king_attack(const Square sq) const;

        typedef Bitboard (Position::*AttackFunction)(const Square sq) const;
        typedef Bitboard (*AttackFunctionBB)(const Square sq, const Bitboard bb);
        static AttackFunction attack[14];
        static AttackFunctionBB attack_bb[14];

        static void init();
        static void init_keys();
        static void init_attack();
        static void init_magic_bb();
        static Bitboard init_magic_bb_occ(const Square* sq, const int numSq, const Bitboard lin);
        static Bitboard init_magic_bb_b(const Square sq, const Bitboard occ);
        static Bitboard init_magic_bb_r(const Square sq, const Bitboard occ);
        static void init_pvt();
        static void init_direction();

        void move_quiet_do(const Move move);
        void move_quiet_undo(const Move move);
        void move_capture_do(const Move move);
        void move_capture_undo(const Move move);
        void move_ep_do(const Move move);
        void move_ep_undo(const Move move);
        void move_castle_do(const Move move);
        void move_castle_undo(const Move move);
        void move_promotion_do(const Move move);
        void move_promotion_undo(const Move move);
        void move_null_do();

        bool wk_castle() const;
        bool wq_castle() const;
        bool bk_castle() const;
        bool bq_castle() const;

        inline bool sq_attacked(const Square sq, const Color c) const;
        inline bool sq_attacked(const Square sq, const Color c, const Bitboard bb) const;
        inline bool sq_attacked(const Square sq, const Square from, const Color c) const;
        inline bool is_attacked(const Square from, const Square to) const;
        inline Bitboard get_checkers() const;
        inline bool is_pinned(const Square from, const Square to) const;
        inline bool is_pinned(const Square from, const Square to, const Square cSq) const;

        Bitboard discover_check(const Square sq) const;

        void eval_pieces(EvalInfo & info,  const MaterialHash* mh) const;
        void eval_patterns(EvalInfo& infol) const;
        void eval_pawns(PawnHash* ph) const;
        void eval_king(EvalInfo& info, const MaterialHash* mh) const;
        void eval_passer(EvalInfo& info, const PawnHash* ph) const;
        void eval_mat(MaterialHash* mh) const;
        void eval_draw(EvalInfo & info,  const MaterialHash* mh) const;
        static int quad(const int min, const int max, const int x);
        int shelter_sq(const Square sq, const Color c) const;
        int storm_file(const int f, const int c) const;
        int shelter_file(const int f, const int r, const int c) const;
        int pawn_att_dist(const Square sq, const Square kSq, const Color c)const;
        int pawn_def_dist(const Square sq, const Square kSq, const Color c)const;
        bool unstoppable_passer(Square sq, const Color c) const;
        bool king_passer(const Square sq, const Color c) const;
        bool free_passer(const Square from , const Color c) const;
        Square pawn_promote(const Square sq, const Color c) const;
        bool bishop_can_attack(const Square to, const Color c) const;
        void draw_list(Square list[8], const Color c) const;
        bool draw_KPKQ(const Color c, const Square list[8]) const;
        bool draw_KPKR(const Color c, const Square list[8]) const;
        bool draw_KPKB(const Square list[8]) const;
        bool draw_KPKN(const Square list[8]) const;
        bool draw_KNPK(const Square list[8]) const;
        bool draw_KRPKR(const Square list[8]) const;
        bool draw_KBPKB(const Square list[8]) const;

         /* pvt */
        static const int Pawn_File[8];
        static const int Pawn_Rank[8];
        static const int Knight_Line[8];
        static const int Knight_Rank[8];
        static const int Bishop_Line[8];
        static const int Rook_File[8];
        static const int Queen_Line[8];
        static const int King_File[8];
        static const int King_Rank[8];
        static const int King_Line[8];
        static const int Pawn_File_Opening;
        static const int Pawn_Rank_Endgame;
        static const int Knight_Center[2];
        static const int Knight_Rank_Opening;
        static const int Knight_Back_Rank_Opening;
        static const int Knight_Trapped;
        static const int Bishop_Center[2];
        static const int Bishop_Back_Rank_Opening;
        static const int Bishop_Diagonal_Opening;
        static const int Rook_File_Opening;
        static const int Queen_Center[2];
        static const int Queen_Back_Rank_Opening;
        static const int King_File_Opening;
        static const int King_Rank_Opening;
        static const int King_Center_Endgame;

        /* eval */
        static const int MAT[12][2];
        static const int ExchangePenalty[2];
        static const int BishopPair[2];

        static const int TrappedBishop;
        static const int BlockedBishop;
        static const int BlockedRook;
        static const int BlockedCenterPawn;

        static const int KnightUnit;
        static const int BishopUnit;
        static const int RookUnit;

        static const int KnightMob[2];
        static const int BishopMob[2];
        static const int RookMob[2];
        static const int QueenMob[2];
        static const int KingMob[2];
        static const int RookSemiOpenFile[2];
        static const int RookOpenFile[2];
        static const int RookSemiKingFileOpening;
        static const int RookKingFileOpening;
        static const int Rook7th[2];
        static const int Queen7th[2];
        static const int StormOpening;
        static const int KingAttackOpening;
        static const int Doubled[2];
        static const int Isolated[2];
        static const int IsolatedOpenOpening;
        static const int Backward[2];
        static const int BackwardOpenOpening;
        static const int CandidateMin[2];
        static const int CandidateMax[2];
        static const int PassedMin[2];
        static const int PassedMax[2];
        static const int ProtectedPassed[2];
        static const int UnstoppablePasser;
        static const int FreePasser;
        static const int AttackerDistance;
        static const int DefenderDistance;
        static const int ShelterOpening;

        static const int KingAttackUnit[14];
        static const int PiecePhase[12];
        static const int KingAttackWeight[16];
        static const int KnightOutpost[2][64];
        static const int Bonus[8];
        static const int SSpaceWeight[16];

        static const int TotalPhase;

        static const int DrawNodeFlag;
        static const int DrawBishopFlag;
        static const int MatRookPawnFlag;
        static const int MatBishopFlag;
        static const int MatKnightFlag;
        static const int MatKingFlag;
    };

inline Key Position::get_key() const{
    return current->key;
}

inline Key Position::get_pawn_key() const{
    return current->pawn_key;
}

inline Key Position::get_mat_key() const{
    return current->mat_key;
}

inline Piece Position::get_piece(const Square sq) const{
    return piece[sq];
}

inline Bitboard Position::knight_attack(const Square sq, const Bitboard){
    return KnightAttack[sq];
}

inline Bitboard Position::knight_attack(const Square sq) const{
    return KnightAttack[sq];
}

inline Bitboard Position::bishop_attack(const Square sq, const Bitboard bb){
    return *(magic_bb_b_indices[sq]+
                        (((bb&magic_bb_b_mask[sq])*magic_bb_b_magics[sq])>>magic_bb_b_shift[sq]));
}

inline Bitboard Position::bishop_attack(const Square sq) const{
    return bishop_attack(sq,occupied);
}

inline Bitboard Position::rook_attack(const Square sq, const Bitboard bb){
    return *(magic_bb_r_indices[sq]+
                        (((bb&magic_bb_r_mask[sq])*magic_bb_r_magics[sq])>>magic_bb_r_shift[sq]));
}

inline Bitboard Position::rook_attack(const Square sq) const{
    return rook_attack(sq,occupied);
}

inline Bitboard Position::queen_attack(const Square sq) const{
    return rook_attack(sq) | bishop_attack(sq);
}

inline Bitboard Position::queen_attack(const Square sq, const Bitboard bb){
    return rook_attack(sq,bb) | bishop_attack(sq,bb);
}

inline Bitboard Position::king_attack(const Square sq) const{
    return KingAttack[sq];
}

inline bool Position::sq_attacked(const Square sq, const Color c) const{
    Color opp = c ^ 1;
    return (rook_attack(sq)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) ||
           (bishop_attack(sq)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp])) ||
           (KnightAttack[sq]&bitboard[KNIGHT+opp]) ||
           (PawnAttack[c][sq]&bitboard[PAWN+opp]);
}

inline bool Position::sq_attacked(const Square sq, const Color c, const Bitboard bb) const{
    Color opp = c ^ 1;
    return (rook_attack(sq,bb)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) ||
           (bishop_attack(sq,bb)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp])) ||
           (KnightAttack[sq]&bitboard[KNIGHT+opp]) ||
           (PawnAttack[c][sq]&bitboard[PAWN+opp]);
}

inline bool Position::is_attacked(const Square from, const Square to)const {
    const int opp = stm ^ 1;
    Bitboard occ = occupied ^ (BB_1<<from);
    return (rook_attack(to,occ)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) ||
           (bishop_attack(to,occ)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp])) ||
           (KnightAttack[to]&bitboard[KNIGHT+opp]) ||
           (PawnAttack[stm][to]&bitboard[PAWN+opp]);
}

inline bool Position::is_pinned(const Square from, const Square to) const{
    const int opp = stm ^ 1;
    const Square kSq = first_bit(bitboard[KING+stm]);
    Bitboard bb_to = BB_1<<to;
    if(bb_to & occupied){
        Bitboard occ = occupied ^ (BB_1<<from);
        Bitboard bb_opp = ~bb_to;
        return (rook_attack(kSq,occ)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])&bb_opp) ||
               (bishop_attack(kSq,occ)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp])&bb_opp);
    }
    else{
        Bitboard occ = occupied ^ (BB_1<<from | BB_1<<to);
        return (rook_attack(kSq,occ)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) ||
               (bishop_attack(kSq,occ)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp]));
    }
}

inline bool Position::is_pinned(const Square from, const Square to, const Square cSq) const{
    const int opp = stm ^ 1;
    const Square kSq = first_bit(bitboard[KING+stm]);
    Bitboard occ = occupied ^ (BB_1<<from | BB_1<<to | BB_1 << cSq);
    return (rook_attack(kSq,occ)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) ||
           (bishop_attack(kSq,occ)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp]));
}

inline Bitboard Position::get_checkers() const{
    int opp = stm ^ 1;
    const Square sq = first_bit(bitboard[KING+stm]);
    return (rook_attack(sq)&(bitboard[ROOK+opp]|bitboard[QUEEN+opp])) |
           (bishop_attack(sq)&(bitboard[BISHOP+opp]|bitboard[QUEEN+opp])) |
           (KnightAttack[sq]&bitboard[KNIGHT+opp]) |
           (PawnAttack[stm][sq]&bitboard[PAWN+opp]);
}

inline bool Position::is_in_check() const{
    return current->checkers != 0;
}

inline int Position::get_stm() const{
    return stm;
}

inline int Position::num_pieces() const{
    return num[White]+num[Black];
}

inline bool Position::egbb() const{
    Piece p = piece[move_to(ph[ply-1].move)];
	return current->capture != NO_PIECE || (p == WP || p == BP);
}

inline bool Position::doNullMove() const{
	return (num[stm] - num[WP+stm]) >= 2;
}

inline Square Position::get_ep() const {
	return current->ep;
}

inline Piece Position::get_captured() const {
	return current->capture;
}

#endif // POSITION_H
