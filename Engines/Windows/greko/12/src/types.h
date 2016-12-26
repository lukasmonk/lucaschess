//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  types.h: chess types and enums, standard headers
//  modified: 01-Oct-2014

#ifndef TYPES_H
#define TYPES_H

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER
typedef unsigned __int8   U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
typedef unsigned __int64 U64;
typedef __int8            I8;
typedef __int16          I16;
typedef __int32          I32;
typedef __int64          I64;
#define LL(x) x##L
#else
#include <stdint.h>
typedef uint8_t           U8;
typedef uint16_t         U16;
typedef uint32_t         U32;
typedef uint64_t         U64;
typedef int8_t            I8;
typedef int16_t          I16;
typedef int32_t          I32;
typedef int64_t          I64;
#define LL(x) x##LL
#endif

typedef U8 PIECE;
typedef U8 COLOR;
typedef U8 FLD;
typedef I16 EVAL;
typedef U64 NODES;

enum PIECE_T
{
	NOPIECE =  0,
	PW      =  2,
	PB      =  3,
	NW      =  4,
	NB      =  5,
	BW      =  6,
	BB      =  7,
	RW      =  8,
	RB      =  9,
	QW      = 10,
	QB      = 11,
	KW      = 12,
	KB      = 13
};

enum COLOR_T { WHITE = 0, BLACK = 1 };

enum FLD_T
{
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1,
	NF
};

enum DIR_T
{
	DIR_R  = 0,
	DIR_UR = 1,
	DIR_U  = 2,
	DIR_UL = 3,
	DIR_L  = 4,
	DIR_DL = 5,
	DIR_D  = 6,
	DIR_DR = 7,
	DIR_NO = 8
};

enum CASTLE_T
{
	WHITE_O_O   = 0x01,
	WHITE_O_O_O = 0x02,
	BLACK_O_O   = 0x04,
	BLACK_O_O_O = 0x08,
	WHITE_DID_O_O   = 0x10,
	WHITE_DID_O_O_O = 0x20,
	BLACK_DID_O_O   = 0x40,
	BLACK_DID_O_O_O = 0x80
};

enum PROTOCOL_T { CONSOLE = 0, WINBOARD = 1, UCI = 2 };
extern PROTOCOL_T g_protocol;

#endif

