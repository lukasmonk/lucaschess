/**
 *magicmoves.h
 *
 *Source file for magic move bitboard generation.
 *
 *See header file for instructions on usage.
 *
 *The magic keys are not optimal for all squares but they are very close
 *to optimal.
 *
 *Copyright (C) 2007 Pradyumna Kannan.
 *
 *This code is provided 'as-is', without any express or implied warranty.
 *In no event will the authors be held liable for any damages arising from
 *the use of this code. Permission is granted to anyone to use this
 *code for any purpose, including commercial applications, and to alter
 *it and redistribute it freely, subject to the following restrictions:
 *
 *1. The origin of this code must not be misrepresented; you must not
 *claim that you wrote the original code. If you use this code in a
 *product, an acknowledgment in the product documentation would be
 *appreciated but is not required.
 *
 *2. Altered source versions must be plainly marked as such, and must not be
 *misrepresented as being the original code.
 *
 *3. This notice may not be removed or altered from any source distribution.
 */

#include "magicmoves.h"

#ifdef _MSC_VER
//	#pragma message("MSC compatible compiler detected -- turning off warning 4312,4146")
//	#pragma warning( disable : 4312)
//	#pragma warning( disable : 4146)
#endif

//For rooks

//original 12 bit keys
//C64(0x0000002040810402) - H8 12 bit
//C64(0x0000102040800101) - A8 12 bit
//C64(0x0000102040008101) - B8 11 bit
//C64(0x0000081020004101) - C8 11 bit

//Adapted Grant Osborne's keys
//C64(0x0001FFFAABFAD1A2) - H8 11 bit
//C64(0x00FFFCDDFCED714A) - A8 11 bit
//C64(0x007FFCDDFCED714A) - B8 10 bit
//C64(0x003FFFCDFFD88096) - C8 10 bit

// **** caution file modified by HJ ****
// see Pradu's site for the original file
// deleted perfect magic parts, and not minimized

// for 32bits friendly multiply see http://www.vpittlik.org/wbforum/viewtopic.php?t=5997
#ifndef USE_32_BIT_MULTIPLICATIONS
const U64 magicmoves_r_mask[64]=
{	
	C64(0x000101010101017E), C64(0x000202020202027C), C64(0x000404040404047A), C64(0x0008080808080876),
	C64(0x001010101010106E), C64(0x002020202020205E), C64(0x004040404040403E), C64(0x008080808080807E),
	C64(0x0001010101017E00), C64(0x0002020202027C00), C64(0x0004040404047A00), C64(0x0008080808087600),
	C64(0x0010101010106E00), C64(0x0020202020205E00), C64(0x0040404040403E00), C64(0x0080808080807E00),
	C64(0x00010101017E0100), C64(0x00020202027C0200), C64(0x00040404047A0400), C64(0x0008080808760800),
	C64(0x00101010106E1000), C64(0x00202020205E2000), C64(0x00404040403E4000), C64(0x00808080807E8000),
	C64(0x000101017E010100), C64(0x000202027C020200), C64(0x000404047A040400), C64(0x0008080876080800),
	C64(0x001010106E101000), C64(0x002020205E202000), C64(0x004040403E404000), C64(0x008080807E808000),
	C64(0x0001017E01010100), C64(0x0002027C02020200), C64(0x0004047A04040400), C64(0x0008087608080800),
	C64(0x0010106E10101000), C64(0x0020205E20202000), C64(0x0040403E40404000), C64(0x0080807E80808000),
	C64(0x00017E0101010100), C64(0x00027C0202020200), C64(0x00047A0404040400), C64(0x0008760808080800),
	C64(0x00106E1010101000), C64(0x00205E2020202000), C64(0x00403E4040404000), C64(0x00807E8080808000),
	C64(0x007E010101010100), C64(0x007C020202020200), C64(0x007A040404040400), C64(0x0076080808080800),
	C64(0x006E101010101000), C64(0x005E202020202000), C64(0x003E404040404000), C64(0x007E808080808000),
	C64(0x7E01010101010100), C64(0x7C02020202020200), C64(0x7A04040404040400), C64(0x7608080808080800),
	C64(0x6E10101010101000), C64(0x5E20202020202000), C64(0x3E40404040404000), C64(0x7E80808080808000)
};


const U64 magicmoves_b_mask[64]=
{
	C64(0x0040201008040200), C64(0x0000402010080400), C64(0x0000004020100A00), C64(0x0000000040221400),
	C64(0x0000000002442800), C64(0x0000000204085000), C64(0x0000020408102000), C64(0x0002040810204000),
	C64(0x0020100804020000), C64(0x0040201008040000), C64(0x00004020100A0000), C64(0x0000004022140000),
	C64(0x0000000244280000), C64(0x0000020408500000), C64(0x0002040810200000), C64(0x0004081020400000),
	C64(0x0010080402000200), C64(0x0020100804000400), C64(0x004020100A000A00), C64(0x0000402214001400),
	C64(0x0000024428002800), C64(0x0002040850005000), C64(0x0004081020002000), C64(0x0008102040004000),
	C64(0x0008040200020400), C64(0x0010080400040800), C64(0x0020100A000A1000), C64(0x0040221400142200),
	C64(0x0002442800284400), C64(0x0004085000500800), C64(0x0008102000201000), C64(0x0010204000402000),
	C64(0x0004020002040800), C64(0x0008040004081000), C64(0x00100A000A102000), C64(0x0022140014224000),
	C64(0x0044280028440200), C64(0x0008500050080400), C64(0x0010200020100800), C64(0x0020400040201000),
	C64(0x0002000204081000), C64(0x0004000408102000), C64(0x000A000A10204000), C64(0x0014001422400000),
	C64(0x0028002844020000), C64(0x0050005008040200), C64(0x0020002010080400), C64(0x0040004020100800),
	C64(0x0000020408102000), C64(0x0000040810204000), C64(0x00000A1020400000), C64(0x0000142240000000),
	C64(0x0000284402000000), C64(0x0000500804020000), C64(0x0000201008040200), C64(0x0000402010080400),
	C64(0x0002040810204000), C64(0x0004081020400000), C64(0x000A102040000000), C64(0x0014224000000000),
	C64(0x0028440200000000), C64(0x0050080402000000), C64(0x0020100804020000), C64(0x0040201008040200)
};



const unsigned int magicmoves_r_shift[64]=
{
	52, 53, 53, 53, 53, 53, 53, 52,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 53, 53, 53, 53, 53
};

const U64 magicmoves_r_magics[64]=
{
	C64(0x0080001020400080), C64(0x0040001000200040), C64(0x0080081000200080), C64(0x0080040800100080),
	C64(0x0080020400080080), C64(0x0080010200040080), C64(0x0080008001000200), C64(0x0080002040800100),
	C64(0x0000800020400080), C64(0x0000400020005000), C64(0x0000801000200080), C64(0x0000800800100080),
	C64(0x0000800400080080), C64(0x0000800200040080), C64(0x0000800100020080), C64(0x0000800040800100),
	C64(0x0000208000400080), C64(0x0000404000201000), C64(0x0000808010002000), C64(0x0000808008001000),
	C64(0x0000808004000800), C64(0x0000808002000400), C64(0x0000010100020004), C64(0x0000020000408104),
	C64(0x0000208080004000), C64(0x0000200040005000), C64(0x0000100080200080), C64(0x0000080080100080),
	C64(0x0000040080080080), C64(0x0000020080040080), C64(0x0000010080800200), C64(0x0000800080004100),
	C64(0x0000204000800080), C64(0x0000200040401000), C64(0x0000100080802000), C64(0x0000080080801000),
	C64(0x0000040080800800), C64(0x0000020080800400), C64(0x0000020001010004), C64(0x0000800040800100),
	C64(0x0000204000808000), C64(0x0000200040008080), C64(0x0000100020008080), C64(0x0000080010008080),
	C64(0x0000040008008080), C64(0x0000020004008080), C64(0x0000010002008080), C64(0x0000004081020004),
	C64(0x0000204000800080), C64(0x0000200040008080), C64(0x0000100020008080), C64(0x0000080010008080),
	C64(0x0000040008008080), C64(0x0000020004008080), C64(0x0000800100020080), C64(0x0000800041000080),
	C64(0x00FFFCDDFCED714A), C64(0x007FFCDDFCED714A), C64(0x003FFFCDFFD88096), C64(0x0000040810002101),
	C64(0x0001000204080011), C64(0x0001000204000801), C64(0x0001000082000401), C64(0x0001FFFAABFAD1A2)
};

//my original tables for bishops
const unsigned int magicmoves_b_shift[64]=
{
	58, 59, 59, 59, 59, 59, 59, 58,
	59, 59, 59, 59, 59, 59, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 59, 59, 59, 59, 59, 59,
	58, 59, 59, 59, 59, 59, 59, 58
};

const U64 magicmoves_b_magics[64]=
{
	C64(0x0002020202020200), C64(0x0002020202020000), C64(0x0004010202000000), C64(0x0004040080000000),
	C64(0x0001104000000000), C64(0x0000821040000000), C64(0x0000410410400000), C64(0x0000104104104000),
	C64(0x0000040404040400), C64(0x0000020202020200), C64(0x0000040102020000), C64(0x0000040400800000),
	C64(0x0000011040000000), C64(0x0000008210400000), C64(0x0000004104104000), C64(0x0000002082082000),
	C64(0x0004000808080800), C64(0x0002000404040400), C64(0x0001000202020200), C64(0x0000800802004000),
	C64(0x0000800400A00000), C64(0x0000200100884000), C64(0x0000400082082000), C64(0x0000200041041000),
	C64(0x0002080010101000), C64(0x0001040008080800), C64(0x0000208004010400), C64(0x0000404004010200),
	C64(0x0000840000802000), C64(0x0000404002011000), C64(0x0000808001041000), C64(0x0000404000820800),
	C64(0x0001041000202000), C64(0x0000820800101000), C64(0x0000104400080800), C64(0x0000020080080080),
	C64(0x0000404040040100), C64(0x0000808100020100), C64(0x0001010100020800), C64(0x0000808080010400),
	C64(0x0000820820004000), C64(0x0000410410002000), C64(0x0000082088001000), C64(0x0000002011000800),
	C64(0x0000080100400400), C64(0x0001010101000200), C64(0x0002020202000400), C64(0x0001010101000200),
	C64(0x0000410410400000), C64(0x0000208208200000), C64(0x0000002084100000), C64(0x0000000020880000),
	C64(0x0000001002020000), C64(0x0000040408020000), C64(0x0004040404040000), C64(0x0002020202020000),
	C64(0x0000104104104000), C64(0x0000002082082000), C64(0x0000000020841000), C64(0x0000000000208800),
	C64(0x0000000010020200), C64(0x0000000404080200), C64(0x0000040404040400), C64(0x0002020202020200)
};
#endif

#ifndef USE_32_BIT_MULTIPLICATIONS
U64 magicmovesbdb[5248];
const U64* magicmoves_b_indices[64]=
{
	magicmovesbdb+4992, magicmovesbdb+2624,  magicmovesbdb+256,  magicmovesbdb+896,
	magicmovesbdb+1280, magicmovesbdb+1664, magicmovesbdb+4800, magicmovesbdb+5120,
	magicmovesbdb+2560, magicmovesbdb+2656,  magicmovesbdb+288,  magicmovesbdb+928,
	magicmovesbdb+1312, magicmovesbdb+1696, magicmovesbdb+4832, magicmovesbdb+4928,
	magicmovesbdb+0,     magicmovesbdb+128,  magicmovesbdb+320,  magicmovesbdb+960,
	magicmovesbdb+1344, magicmovesbdb+1728, magicmovesbdb+2304, magicmovesbdb+2432,
	magicmovesbdb+32,    magicmovesbdb+160,  magicmovesbdb+448, magicmovesbdb+2752,
	magicmovesbdb+3776, magicmovesbdb+1856, magicmovesbdb+2336, magicmovesbdb+2464,
	magicmovesbdb+64,    magicmovesbdb+192,  magicmovesbdb+576, magicmovesbdb+3264,
	magicmovesbdb+4288, magicmovesbdb+1984, magicmovesbdb+2368, magicmovesbdb+2496,
	magicmovesbdb+96,    magicmovesbdb+224,  magicmovesbdb+704, magicmovesbdb+1088,
	magicmovesbdb+1472, magicmovesbdb+2112, magicmovesbdb+2400, magicmovesbdb+2528,
	magicmovesbdb+2592, magicmovesbdb+2688,  magicmovesbdb+832, magicmovesbdb+1216,
	magicmovesbdb+1600, magicmovesbdb+2240, magicmovesbdb+4864, magicmovesbdb+4960,
	magicmovesbdb+5056, magicmovesbdb+2720,  magicmovesbdb+864, magicmovesbdb+1248,
	magicmovesbdb+1632, magicmovesbdb+2272, magicmovesbdb+4896, magicmovesbdb+5184
};
#endif


#ifndef USE_32_BIT_MULTIPLICATIONS
U64 magicmovesrdb[102400];
const U64* magicmoves_r_indices[64]=
{
	magicmovesrdb+86016, magicmovesrdb+73728, magicmovesrdb+36864, magicmovesrdb+43008,
	magicmovesrdb+47104, magicmovesrdb+51200, magicmovesrdb+77824, magicmovesrdb+94208,
	magicmovesrdb+69632, magicmovesrdb+32768, magicmovesrdb+38912, magicmovesrdb+10240,
	magicmovesrdb+14336, magicmovesrdb+53248, magicmovesrdb+57344, magicmovesrdb+81920,
	magicmovesrdb+24576, magicmovesrdb+33792,  magicmovesrdb+6144, magicmovesrdb+11264,
	magicmovesrdb+15360, magicmovesrdb+18432, magicmovesrdb+58368, magicmovesrdb+61440,
	magicmovesrdb+26624,  magicmovesrdb+4096,  magicmovesrdb+7168,     magicmovesrdb+0,
	 magicmovesrdb+2048, magicmovesrdb+19456, magicmovesrdb+22528, magicmovesrdb+63488,
	magicmovesrdb+28672,  magicmovesrdb+5120,  magicmovesrdb+8192,  magicmovesrdb+1024,
	 magicmovesrdb+3072, magicmovesrdb+20480, magicmovesrdb+23552, magicmovesrdb+65536,
	magicmovesrdb+30720, magicmovesrdb+34816,  magicmovesrdb+9216, magicmovesrdb+12288,
	magicmovesrdb+16384, magicmovesrdb+21504, magicmovesrdb+59392, magicmovesrdb+67584,
	magicmovesrdb+71680, magicmovesrdb+35840, magicmovesrdb+39936, magicmovesrdb+13312,
	magicmovesrdb+17408, magicmovesrdb+54272, magicmovesrdb+60416, magicmovesrdb+83968,
	magicmovesrdb+90112, magicmovesrdb+75776, magicmovesrdb+40960, magicmovesrdb+45056,
	magicmovesrdb+49152, magicmovesrdb+55296, magicmovesrdb+79872, magicmovesrdb+98304
};
#endif

// attacks on an empty board
U64 RAttackEmpty[64];
U64 BAttackEmpty[64];

static void initAttackEmpty(void) {
    // used to compute pins in PinCheckUpdate()
    for(int i = 0; i < 64 ; i++) {
        RAttackEmpty[i] = Rmagic(i, 0);
        BAttackEmpty[i] = Bmagic(i, 0);
    }
}

#ifndef USE_32_BIT_MULTIPLICATIONS

U64 initmagicmoves_occ(const int* squares, const int numSquares, const U64 linocc)
{
	int i;
	U64 ret=0;
	for(i=0;i<numSquares;i++)
		if(linocc&(((U64)(1))<<i)) ret|=(((U64)(1))<<squares[i]);
	return ret;
}

U64 initmagicmoves_Rmoves(const int square, const U64 occ)
{
	U64 ret=0;
	U64 bit;
	U64 rowbits=(((U64)0xFF)<<(8*(square/8)));
	
	bit=(((U64)(1))<<square);
	do
	{
		bit<<=8;
		ret|=bit;
	}while(bit && !(bit&occ));
	bit=(((U64)(1))<<square);
	do
	{
		bit>>=8;
		ret|=bit;
	}while(bit && !(bit&occ));
	bit=(((U64)(1))<<square);
	do
	{
		bit<<=1;
		if(bit&rowbits) ret|=bit;
		else break;
	}while(!(bit&occ));
	bit=(((U64)(1))<<square);
	do
	{
		bit>>=1;
		if(bit&rowbits) ret|=bit;
		else break;
	}while(!(bit&occ));
	return ret;
}

U64 initmagicmoves_Bmoves(const int square, const U64 occ)
{
	U64 ret=0;
	U64 bit;
	U64 bit2;
	U64 rowbits=(((U64)0xFF)<<(8*(square/8)));
	
	bit=(((U64)(1))<<square);
	bit2=bit;
	do
	{
		bit<<=8-1;
		bit2>>=1;
		if(bit2&rowbits) ret|=bit;
		else break;
	}while(bit && !(bit&occ));
	bit=(((U64)(1))<<square);
	bit2=bit;
	do
	{
		bit<<=8+1;
		bit2<<=1;
		if(bit2&rowbits) ret|=bit;
		else break;
	}while(bit && !(bit&occ));
	bit=(((U64)(1))<<square);
	bit2=bit;
	do
	{
		bit>>=8-1;
		bit2<<=1;
		if(bit2&rowbits) ret|=bit;
		else break;
	}while(bit && !(bit&occ));
	bit=(((U64)(1))<<square);
	bit2=bit;
	do
	{
		bit>>=8+1;
		bit2>>=1;
		if(bit2&rowbits) ret|=bit;
		else break;
	}while(bit && !(bit&occ));
	return ret;
}

//used so that the original indices can be left as const so that the compiler can optimize better

#ifdef USE_32_BIT_MULTIPLICATIONS
        #define BmagicNOMASK2(square,occ) \
            *(bIndecies[square] + ((unsigned((int)(occ) * (int)(bishopMagic[square])) ^ \
        unsigned((int)(occ >> 32) * (int)(bishopMagic[square] >> 32))) >> bishopShift[square]))
        #define RmagicNOMASK2(square, occ) \
            *(rIndecies[square] + ((unsigned((int)(occ) * (int)(rookMagic[square])) ^ \
            unsigned((int)(occ >> 32) * (int)(rookMagic[square] >> 32))) >> rookShift[square]))

#else
		#define BmagicNOMASK2(square, occupancy) *(magicmoves_b_indices2[square]+(((occupancy)*magicmoves_b_magics[square])>>magicmoves_b_shift[square]))
		#define RmagicNOMASK2(square, occupancy) *(magicmoves_r_indices2[square]+(((occupancy)*magicmoves_r_magics[square])>>magicmoves_r_shift[square]))
#endif



void initmagicmoves(void)
{
	int i;

	//for bitscans :
	//initmagicmoves_bitpos64_database[(x*C64(0x07EDD5E59A4E28C2))>>58]
	int initmagicmoves_bitpos64_database[64]={
	63,  0, 58,  1, 59, 47, 53,  2,
	60, 39, 48, 27, 54, 33, 42,  3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22,  4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16,  9, 12,
	44, 24, 15,  8, 23,  7,  6,  5};

#ifndef USE_32_BIT_MULTIPLICATIONS
	//identical to magicmove_x_indices except without the const modifer
	U64* magicmoves_b_indices2[64]=
	{
		magicmovesbdb+4992, magicmovesbdb+2624,  magicmovesbdb+256,  magicmovesbdb+896,
		magicmovesbdb+1280, magicmovesbdb+1664, magicmovesbdb+4800, magicmovesbdb+5120,
		magicmovesbdb+2560, magicmovesbdb+2656,  magicmovesbdb+288,  magicmovesbdb+928,
		magicmovesbdb+1312, magicmovesbdb+1696, magicmovesbdb+4832, magicmovesbdb+4928,
		magicmovesbdb+0,     magicmovesbdb+128,  magicmovesbdb+320,  magicmovesbdb+960,
		magicmovesbdb+1344, magicmovesbdb+1728, magicmovesbdb+2304, magicmovesbdb+2432,
		magicmovesbdb+32,    magicmovesbdb+160,  magicmovesbdb+448, magicmovesbdb+2752,
		magicmovesbdb+3776, magicmovesbdb+1856, magicmovesbdb+2336, magicmovesbdb+2464,
		magicmovesbdb+64,    magicmovesbdb+192,  magicmovesbdb+576, magicmovesbdb+3264,
		magicmovesbdb+4288, magicmovesbdb+1984, magicmovesbdb+2368, magicmovesbdb+2496,
		magicmovesbdb+96,    magicmovesbdb+224,  magicmovesbdb+704, magicmovesbdb+1088,
		magicmovesbdb+1472, magicmovesbdb+2112, magicmovesbdb+2400, magicmovesbdb+2528,
		magicmovesbdb+2592, magicmovesbdb+2688,  magicmovesbdb+832, magicmovesbdb+1216,
		magicmovesbdb+1600, magicmovesbdb+2240, magicmovesbdb+4864, magicmovesbdb+4960,
		magicmovesbdb+5056, magicmovesbdb+2720,  magicmovesbdb+864, magicmovesbdb+1248,
		magicmovesbdb+1632, magicmovesbdb+2272, magicmovesbdb+4896, magicmovesbdb+5184
	};
	U64* magicmoves_r_indices2[64]=
	{
		magicmovesrdb+86016, magicmovesrdb+73728, magicmovesrdb+36864, magicmovesrdb+43008,
		magicmovesrdb+47104, magicmovesrdb+51200, magicmovesrdb+77824, magicmovesrdb+94208,
		magicmovesrdb+69632, magicmovesrdb+32768, magicmovesrdb+38912, magicmovesrdb+10240,
		magicmovesrdb+14336, magicmovesrdb+53248, magicmovesrdb+57344, magicmovesrdb+81920,
		magicmovesrdb+24576, magicmovesrdb+33792,  magicmovesrdb+6144, magicmovesrdb+11264,
		magicmovesrdb+15360, magicmovesrdb+18432, magicmovesrdb+58368, magicmovesrdb+61440,
		magicmovesrdb+26624,  magicmovesrdb+4096,  magicmovesrdb+7168,     magicmovesrdb+0,
		magicmovesrdb+2048,  magicmovesrdb+19456, magicmovesrdb+22528, magicmovesrdb+63488,
		magicmovesrdb+28672,  magicmovesrdb+5120,  magicmovesrdb+8192,  magicmovesrdb+1024,
		magicmovesrdb+3072,  magicmovesrdb+20480, magicmovesrdb+23552, magicmovesrdb+65536,
		magicmovesrdb+30720, magicmovesrdb+34816,  magicmovesrdb+9216, magicmovesrdb+12288,
		magicmovesrdb+16384, magicmovesrdb+21504, magicmovesrdb+59392, magicmovesrdb+67584,
		magicmovesrdb+71680, magicmovesrdb+35840, magicmovesrdb+39936, magicmovesrdb+13312,
		magicmovesrdb+17408, magicmovesrdb+54272, magicmovesrdb+60416, magicmovesrdb+83968,
		magicmovesrdb+90112, magicmovesrdb+75776, magicmovesrdb+40960, magicmovesrdb+45056,
		magicmovesrdb+49152, magicmovesrdb+55296, magicmovesrdb+79872, magicmovesrdb+98304
	};
#endif


	for(i=0;i<64;i++)
	{
		int squares[64];
		int numsquares=0;
		U64 temp=magicmoves_b_mask[i];
		while(temp)
		{
			U64 bit=temp&-temp;
			squares[numsquares++]=initmagicmoves_bitpos64_database[(bit*C64(0x07EDD5E59A4E28C2))>>58];
			temp^=bit;
		}
		for(temp=0;temp<(((U64)(1))<<numsquares);temp++)
		{
			U64 tempocc=initmagicmoves_occ(squares,numsquares,temp);
            BmagicNOMASK2(i,tempocc)=initmagicmoves_Bmoves(i,tempocc);
		}
	}
	for(i=0;i<64;i++)
	{
		int squares[64];
		int numsquares=0;
		U64 temp=magicmoves_r_mask[i];
		while(temp)
		{
			U64 bit=temp&-temp;
			squares[numsquares++]=initmagicmoves_bitpos64_database[(bit*C64(0x07EDD5E59A4E28C2))>>58];
			temp^=bit;
		}
		for(temp=0;temp<(((U64)(1))<<numsquares);temp++)
		{
			U64 tempocc=initmagicmoves_occ(squares,numsquares,temp);
				RmagicNOMASK2(i,tempocc)=initmagicmoves_Rmoves(i,tempocc);
		}
	}

    initAttackEmpty();

}
#endif

#ifdef USE_32_BIT_MULTIPLICATIONS
// magics from Glaurung until I can generate some
const U64 RMult[64] = {
  0xd7445cdec88002c0ULL, 0xd0a505c1f2001722ULL, 0xe065d1c896002182ULL,
  0x9a8c41e75a000892ULL, 0x8900b10c89002aa8ULL, 0x9b28d1c1d60005a2ULL,
  0x15d6c88de002d9aULL, 0xb1dbfc802e8016a9ULL, 0x149a1042d9d60029ULL,
  0xb9c08050599e002fULL, 0x132208c3af300403ULL, 0xc1000ce2e9c50070ULL,
  0x9d9aa13c99020012ULL, 0xb6b078daf71e0046ULL, 0x9d880182fb6e002eULL,
  0x52889f467e850037ULL, 0xda6dc008d19a8480ULL, 0x468286034f902420ULL,
  0x7140ac09dc54c020ULL, 0xd76ffffa39548808ULL, 0xea901c4141500808ULL,
  0xc91004093f953a02ULL, 0x2882afa8f6bb402ULL, 0xaebe335692442c01ULL,
  0xe904a22079fb91eULL, 0x13a514851055f606ULL, 0x76c782018c8fe632ULL,
  0x1dc012a9d116da06ULL, 0x3c9e0037264fffa6ULL, 0x2036002853c6e4a2ULL,
  0xe3fe08500afb47d4ULL, 0xf38af25c86b025c2ULL, 0xc0800e2182cf9a40ULL,
  0x72002480d1f60673ULL, 0x2500200bae6e9b53ULL, 0xc60018c1eefca252ULL,
  0x600590473e3608aULL, 0x46002c4ab3fe51b2ULL, 0xa200011486bcc8d2ULL,
  0xb680078095784c63ULL, 0x2742002639bf11aeULL, 0xc7d60021a5bdb142ULL,
  0xc8c04016bb83d820ULL, 0xbd520028123b4842ULL, 0x9d1600344ac2a832ULL,
  0x6a808005631c8a05ULL, 0x604600a148d5389aULL, 0xe2e40103d40dea65ULL,
  0x945b5a0087c62a81ULL, 0x12dc200cd82d28eULL, 0x2431c600b5f9ef76ULL,
  0xfb142a006a9b314aULL, 0x6870e00a1c97d62ULL, 0x2a9db2004a2689a2ULL,
  0xd3594600caf5d1a2ULL, 0xee0e4900439344a7ULL, 0x89c4d266ca25007aULL,
  0x3e0013a2743f97e3ULL, 0x180e31a0431378aULL, 0x3a9e465a4d42a512ULL,
  0x98d0a11a0c0d9cc2ULL, 0x8e711c1aba19b01eULL, 0x8dcdc836dd201142ULL,
  0x5ac08a4735370479ULL,
};

#if 0
const U64 RMult[64] = {
  0x28100040ULL,  0x8400010ULL,  0x202048ULL,  0x800888ULL,
  0x1a000802ULL,  0x5002224ULL,  0x6000182ULL,  0x1001041ULL,
  0x2020ULL,  0x4020ULL,  0x802020ULL,  0x841010ULL,
  0x1201011ULL,  0x4022ULL,  0x2010011ULL,  0x102001ULL,
  0x842280ULL,  0x18400210ULL,  0x40020ULL,  0xc41010ULL,
  0x20022204ULL,  0x8404041ULL,  0x101ULL,  0x4000201ULL,
  0x100a5ULL,  0x8210321ULL,  0x1261ULL,  0x4a041001ULL,
  0x40002901ULL,  0x204080eULL,  0x40040401ULL,  0x3280482ULL,
  0x20801040ULL,  0x20400812ULL,  0xc0061ULL,  0x1200804ULL,
  0x24188ULL,  0x8101a13ULL,  0x18080802ULL,  0x64000041ULL,
  0x21800120ULL,  0x8480bULL,  0x1100241ULL,  0x110248ULL,
  0x16010004ULL,  0xc6142ULL,  0x4020410aULL,  0x4580041ULL,
  0x406180ULL,  0x2040a3ULL,  0xa002804ULL,  0x10010b2ULL,
  0x12a1304ULL,  0x1012ULL,  0xe840482ULL,  0x4885ULL,
  0x10001041ULL,  0x2084250ULL,  0x14619ULL,  0x10000c23ULL,
  0x860422ULL,  0x90b2004ULL,  0x24410282ULL,  0x1000443ULL,
};
#endif
const int RShift[64] = {
  20, 21, 21, 21, 21, 21, 21, 20, 21, 22, 22, 22, 22, 22, 22, 21,
  21, 22, 22, 22, 22, 22, 22, 21, 21, 22, 22, 22, 22, 22, 22, 21,
  21, 22, 22, 22, 22, 22, 22, 21, 21, 22, 22, 22, 22, 22, 22, 21,
  21, 22, 22, 22, 22, 22, 22, 21, 20, 21, 21, 21, 21, 21, 21, 20
};

const U64 BMult[64] = {
  0x54142844c6a22981ULL, 0x710358a6ea25c19eULL, 0x704f746d63a4a8dcULL,
  0xbfed1a0b80f838c5ULL, 0x90561d5631e62110ULL, 0x2804260376e60944ULL,
  0x84a656409aa76871ULL, 0xf0267f64c28b6197ULL, 0x70764ebb762f0585ULL,
  0x92aa09e0cfe161deULL, 0x41ee1f6bb266f60eULL, 0xddcbf04f6039c444ULL,
  0x5a3fab7bac0d988aULL, 0xd3727877fa4eaa03ULL, 0xd988402d868ddaaeULL,
  0x812b291afa075c7cULL, 0x94faf987b685a932ULL, 0x3ed867d8470d08dbULL,
  0x92517660b8901de8ULL, 0x2d97e43e058814b4ULL, 0x880a10c220b25582ULL,
  0xc7c6520d1f1a0477ULL, 0xdbfc7fbcd7656aa6ULL, 0x78b1b9bfb1a2b84fULL,
  0x2f20037f112a0bc1ULL, 0x657171ea2269a916ULL, 0xc08302b07142210eULL,
  0x880a4403064080bULL, 0x3602420842208c00ULL, 0x852800dc7e0b6602ULL,
  0x595a3fbbaa0f03b2ULL, 0x9f01411558159d5eULL, 0x2b4a4a5f88b394f2ULL,
  0x4afcbffc292dd03aULL, 0x4a4094a3b3f10522ULL, 0xb06f00b491f30048ULL,
  0xd5b3820280d77004ULL, 0x8b2e01e7c8e57a75ULL, 0x2d342794e886c2e6ULL,
  0xc302c410cde21461ULL, 0x111f426f1379c274ULL, 0xe0569220abb31588ULL,
  0x5026d3064d453324ULL, 0xe2076040c343cd8aULL, 0x93efd1e1738021eeULL,
  0xb680804bed143132ULL, 0x44e361b21986944cULL, 0x44c60170ef5c598cULL,
  0xf4da475c195c9c94ULL, 0xa3afbb5f72060b1dULL, 0xbc75f410e41c4ffcULL,
  0xb51c099390520922ULL, 0x902c011f8f8ec368ULL, 0x950b56b3d6f5490aULL,
  0x3909e0635bf202d0ULL, 0x5744f90206ec10ccULL, 0xdc59fd76317abbc1ULL,
  0x881c7c67fcbfc4f6ULL, 0x47ca41e7e440d423ULL, 0xeb0c88112048d004ULL,
  0x51c60e04359aef1aULL, 0x1aa1fe0e957a5554ULL, 0xdd9448db4f5e3104ULL,
  0xdc01f6dca4bebbdcULL,
}; 

#if 0
const U64 BMult[64] = {
  0x51a00401ULL,  0x12224a02ULL,  0x5412542ULL,  0x20c40502ULL,
  0x50040308ULL,  0x10020230ULL,  0x2014417ULL,  0x1310208aULL,
  0x2016172ULL,  0x44100e08ULL,  0x10040808ULL,  0x484405ULL,
  0x120aULL,  0x2583420aULL,  0x84c0051ULL,  0x8030692ULL,
  0x45020ULL,  0x52122cULL,  0x100803ULL,  0x610032ULL,
  0x41604002ULL,  0x10020001ULL,  0x20025204ULL,  0x20826001ULL,
  0x131801ULL,  0x842100ULL,  0x20080240ULL,  0x2202008ULL,
  0x500401ULL,  0x62080131ULL,  0x14022082ULL,  0x20804b00ULL,
  0x80210ULL,  0x12084210ULL,  0x60904ULL,  0x20500c14ULL,
  0x204404ULL,  0x212081ULL,  0x19244184ULL,  0x10030841ULL,
  0x11080a02ULL,  0x414c8ULL,  0x401058ULL,  0x40000105ULL,
  0x40044421ULL,  0x2848110aULL,  0x10300101ULL,  0x8012304ULL,
  0x2014a02ULL,  0x5104208ULL,  0x48200111ULL,  0x3000802ULL,
  0x34011420ULL,  0x5004434ULL,  0x4080204ULL,  0x50041802ULL,
  0x28441145ULL,  0x2802018aULL,  0x5500801ULL,  0x58180a00ULL,
  0x20024020ULL,  0x1144112ULL,  0x5030ULL,  0x21020424ULL,
};
#endif

const int BShift[64] = {
  26, 27, 27, 27, 27, 27, 27, 26, 27, 27, 27, 27, 27, 27, 27, 27,
  27, 27, 25, 25, 25, 25, 27, 27, 27, 27, 25, 23, 23, 25, 27, 27,
  27, 27, 25, 23, 23, 25, 27, 27, 27, 27, 25, 25, 25, 25, 27, 27,
  27, 27, 27, 27, 27, 27, 27, 27, 26, 27, 27, 27, 27, 27, 27, 26
};

typedef U64 Bitboard;

Bitboard RMask[64];
int RAttackIndex[64];
Bitboard RAttacks[0x19000];

Bitboard BMask[64];
int BAttackIndex[64];
Bitboard BAttacks[0x1480];

static const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2, 
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52, 
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28, 
  58, 20, 37, 17, 36, 8
};


static int pop_1st_bit(Bitboard* b) {
  Bitboard bb = *b ^ (*b - 1);
  unsigned int fold = int(bb) ^ int(bb >> 32);
  *b &= (*b - 1);
  return BitTable[(fold * 0x783a9b23) >> 26];
}

static Bitboard index_to_bitboard(int index, int bits, Bitboard mask) {
    int i, j;
    Bitboard result = 0ULL;
    for(i = 0; i < bits; i++) {
      j = pop_1st_bit(&mask);
      if(index & (1 << i)) result |= (1ULL << j);
    }
    return result;
  }

  
static Bitboard rook_mask(int sq) {
    Bitboard result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
    for(r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
    for(f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
    for(f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
    return result;
  }
    

static void init_rook_masks() {
    for(int i = 0; i < 64; i++) RMask[i] = rook_mask(i);
  }

  
static Bitboard rook_attacks(int sq, Bitboard block) {
    Bitboard result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r = rk+1; r <= 7; r++) {
      result |= (1ULL << (fl + r*8));
      if(block & (1ULL << (fl + r*8))) break;
    }
    for(r = rk-1; r >= 0; r--) {
      result |= (1ULL << (fl + r*8));
      if(block & (1ULL << (fl + r*8))) break;
    }
    for(f = fl+1; f <= 7; f++) {
      result |= (1ULL << (f + rk*8));
      if(block & (1ULL << (f + rk*8))) break;
    }
    for(f = fl-1; f >= 0; f--) {
      result |= (1ULL << (f + rk*8));
      if(block & (1ULL << (f + rk*8))) break;
    }
    return result;
  }
  
    
static void init_rook_attacks() {
    int i, j, k, index = 0;
    Bitboard b;
    for(i = 0; i < 64; i++) {
      RAttackIndex[i] = index;
      j = (1 << (64 - RShift[i]));
      for(k = 0; k < j; k++) {
        b = index_to_bitboard(k, 32 - RShift[i], RMask[i]);
        RAttacks[index + 
                 (unsigned(int(b) * int(RMult[i]) ^ 
                           int(b >> 32) * int(RMult[i] >> 32)) 
                  >> RShift[i])] =
          rook_attacks(i, b);
/*        RMobility[index + 
                  (unsigned(int(b) * int(RMult[i]) ^ 
                            int(b >> 32) * int(RMult[i] >> 32)) 
                   >> RShift[i])] =
          count_1s(rook_attacks(i, b));*/
      }
      index += j;
    }
  }

  
static Bitboard bishop_mask(int sq) {
    Bitboard result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r = rk+1, f = fl+1; r <= 6 && f <= 6; r++, f++)
      result |= (1ULL << (f + r*8));
    for(r = rk+1, f = fl-1; r <= 6 && f >= 1; r++, f--)
      result |= (1ULL << (f + r*8));
    for(r = rk-1, f = fl+1; r >= 1 && f <= 6; r--, f++)
      result |= (1ULL << (f + r*8));
    for(r = rk-1, f = fl-1; r >= 1 && f >= 1; r--, f--)
      result |= (1ULL << (f + r*8));
    return result;
  }

  
static void init_bishop_masks() {
    for(int i = 0; i < 64; i++) BMask[i] = bishop_mask(i);
  }

  
static Bitboard bishop_attacks(int sq, Bitboard block) {
    Bitboard result = 0ULL;
    int rk = sq/8, fl = sq%8, r, f;
    for(r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
      result |= (1ULL << (f + r*8));
      if(block & (1ULL << (f + r * 8))) break;
    }
    for(r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
      result |= (1ULL << (f + r*8));
      if(block & (1ULL << (f + r * 8))) break;
    }
    for(r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
      result |= (1ULL << (f + r*8));
      if(block & (1ULL << (f + r * 8))) break;
    }
    for(r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
      result |= (1ULL << (f + r*8));
      if(block & (1ULL << (f + r * 8))) break;
    }
    return result;
  }

  
static void init_bishop_attacks() {
    int i, j, k, index = 0;
    Bitboard b;
    for(i = 0; i < 64; i++) {
      BAttackIndex[i] = index;
      j = (1 << (64 - BShift[i]));
      for(k = 0; k < j; k++) {
        b = index_to_bitboard(k, 32 - BShift[i], BMask[i]);
        BAttacks[index + 
                 (unsigned(int(b) * int(BMult[i]) ^ 
                           int(b >> 32) * int(BMult[i] >> 32)) 
                  >> BShift[i])] =
          bishop_attacks(i, b);
/*        BMobility[index + 
                  (unsigned(int(b) * int(BMult[i]) ^ 
                            int(b >> 32) * int(BMult[i] >> 32)) 
                   >> BShift[i])] =
          count_1s(bishop_attacks(i, b));*/
      }
      index += j;
    }
  }

void initmagicmoves(void) {
    init_rook_masks();
    init_bishop_masks();
    init_rook_attacks();
    init_bishop_attacks();
    initAttackEmpty();
}

#endif
