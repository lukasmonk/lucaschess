//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  position.h: position and game representation
//  modified: 19-Dec-2014

#ifndef POSITION_H
#define POSITION_H

#include "bitboards.h"

inline int   Col(int f) { return (f % 8); }
inline int   Row(int f) { return (f / 8); }
inline COLOR GetColor(PIECE p) { return (p & 1); }
inline COLOR Opp(COLOR side) { return (side ^ 1); }

class Move
{
public:
	Move() : m_data(0) {}
	Move(U32 x) : m_data(x) {}
	Move(FLD from, FLD to, PIECE piece) :
		m_data(U32(from) | (U32(to) << 6) | (U32(piece) << 12)) {}
	Move(FLD from, FLD to, PIECE piece, PIECE captured) :
		m_data(U32(from) | (U32(to) << 6) | (U32(piece) << 12) | (U32(captured) << 16)) {}
	Move(FLD from, FLD to, PIECE piece, PIECE captured, PIECE promotion) :
		m_data(U32(from) | (U32(to) << 6) | (U32(piece) << 12) | (U32(captured) << 16) | (U32(promotion) << 20)) {}
	FLD From() const { return m_data & 0x3f; }
	FLD To() const { return (m_data >> 6) & 0x3f; }
	FLD Piece() const { return (m_data >> 12) & 0x0f; }
	FLD Captured() const { return (m_data >> 16) & 0x0f; }
	FLD Promotion() const { return (m_data >> 20) & 0x0f; }
	operator U32() const { return m_data; }
private:
	U32 m_data;
};

class Position
{
public:
	PIECE operator[] (int f) const { return m_board[f]; }
	U64   Bits(PIECE p) const { return m_bits[p]; }
	U64   BitsAll(COLOR side) const { return m_bitsAll[side]; }
	U64   BitsAll() const { return m_bitsAll[WHITE] | m_bitsAll[BLACK]; }
	U8    Castlings() const { return m_castlings; }
	int   Count(PIECE p) const { return m_count[p]; }
	FLD   Ep() const { return m_ep; }
	int   Fifty() const { return m_fifty; }
	U64   GetAttacks(FLD to, COLOR side, U64 occ) const;
	int   GetRepetitions() const;
	std::string Fen() const;
	U64   Hash() const { return m_hash ^ m_castlings ^ m_ep; }
	static void InitHashNumbers();
	bool  InCheck() const { return IsAttacked(King(Side()), Side() ^ 1); }
	bool  IsAttacked(FLD to, COLOR side) const;
	bool  IsGameOver(std::string& message);
	FLD   King(COLOR side) const { return m_Kings[side]; }
	Move  LastMove() const { return (m_undoSize > 0)? m_undos[m_undoSize - 1].m_mv : Move(); }
	bool  MakeMove(Move mv);
	void  MakeNullMove();
	void  Mirror();
	EVAL  Material(COLOR side) const { return m_material[side]; }
	int   MatIndex(COLOR side) const { return m_matIndex[side]; }
	U32   PawnHash() const { return m_hashPawn; }
	int   Ply() const { return m_ply; }
	void  Print() const;
	bool  SetFen(const std::string& fen);
	void  SetInitial() { SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }
	COLOR Side() const { return m_side; }
	void  UnmakeMove();
	void  UnmakeNullMove();
private:
	void Clear();
	void Put(FLD f, PIECE p);
	void Remove(FLD f);

	U64   m_bits[14];
	U64   m_bitsAll[2];
	PIECE m_board[64];
	U8    m_castlings;
	int   m_count[14];
	FLD   m_ep;
	int   m_fifty;
	U64   m_hash;
	U32   m_hashPawn;
	FLD   m_Kings[2];
	EVAL  m_material[2];
	int   m_matIndex[2];
	int   m_ply;
	COLOR m_side;
	struct Undo
	{
		U8   m_castlings;
		FLD  m_ep;
		int  m_fifty;
		U64  m_hash;
		Move m_mv;
	};
	Undo m_undos[1024];
	int m_undoSize;
	static U64 s_hashSide[2];
	static U64 s_hash[64][14];
	static U32 s_hashPawn[64][14];
};

static const int DELTA[14] = { 0, 0, 0, 0, 3, 3, 3, 3, 5, 5, 10, 10, 0, 0 };
extern EVAL VALUE[14];

inline void Position::Put(FLD f, PIECE p)
{
	assert(f <= H1);
	assert(p != NOPIECE);
	assert(m_board[f] == NOPIECE);

	m_board[f] = p;
	m_bits[p] ^= Bitboard::Single(f);
	m_bitsAll[GetColor(p)] ^= Bitboard::Single(f);
	m_hash ^= s_hash[f][p];
	m_hashPawn ^= s_hashPawn[f][p];

	COLOR side = GetColor(p);
	++m_count[p];
	m_matIndex[side] += DELTA[p];
	m_material[side] += VALUE[p];
}

inline void Position::Remove(FLD f)
{
	assert(f <= H1);
	PIECE p = m_board[f];
	assert(p != NOPIECE);

	m_board[f] = NOPIECE;
	m_bits[p] ^= Bitboard::Single(f);
	m_bitsAll[GetColor(p)] ^= Bitboard::Single(f);
	m_hash ^= s_hash[f][p];
	m_hashPawn ^= s_hashPawn[f][p];

	COLOR side = GetColor(p);
	--m_count[p];
	m_matIndex[side] -= DELTA[p];
	m_material[side] -= VALUE[p];
}

const FLD FLIP[64] =
{
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};

#endif
