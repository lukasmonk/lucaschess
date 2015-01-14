#include <iostream>
#include <iomanip>
#include "defines.h"
#include "extglobals.h"
 
unsigned int bitCnt(U64 bitmap)
{
 
	// MIT HAKMEM algorithm, see http://graphics.stanford.edu/~seander/bithacks.html
 
	static const U64  M1 = 0x5555555555555555;  // 1 zero,  1 one ...
	static const U64  M2 = 0x3333333333333333;  // 2 zeros,  2 ones ...
	static const U64  M4 = 0x0f0f0f0f0f0f0f0f;  // 4 zeros,  4 ones ...
	static const U64  M8 = 0x00ff00ff00ff00ff;  // 8 zeros,  8 ones ...
	static const U64 M16 = 0x0000ffff0000ffff;  // 16 zeros, 16 ones ...
	static const U64 M32 = 0x00000000ffffffff;  // 32 zeros, 32 ones
 
	bitmap = (bitmap & M1 ) + ((bitmap >>  1) & M1 );   //put count of each  2 bits into those  2 bits
	bitmap = (bitmap & M2 ) + ((bitmap >>  2) & M2 );   //put count of each  4 bits into those  4 bits
	bitmap = (bitmap & M4 ) + ((bitmap >>  4) & M4 );   //put count of each  8 bits into those  8 bits
	bitmap = (bitmap & M8 ) + ((bitmap >>  8) & M8 );   //put count of each 16 bits into those 16 bits
	bitmap = (bitmap & M16) + ((bitmap >> 16) & M16);   //put count of each 32 bits into those 32 bits
	bitmap = (bitmap & M32) + ((bitmap >> 32) & M32);   //put count of each 64 bits into those 64 bits
	return (int)bitmap;
}
 
unsigned int firstOne(U64 bitmap)
{
	// De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
	// don't use this if bitmap = 0
 
	static const int INDEX64[64] = {
	63,  0, 58,  1, 59, 47, 53,  2,
	60, 39, 48, 27, 54, 33, 42,  3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22,  4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16,  9, 12,
	44, 24, 15,  8, 23,  7,  6,  5  };
 
	static const U64 DEBRUIJN64 = U64(0x07EDD5E59A4E28C2);
 
	// here you would get a warming: "unary minus operator applied to unsigned type",
	// that's intended and OK so I'll disable it
#pragma warning (disable: 4146)
	return INDEX64[((bitmap & -bitmap) * DEBRUIJN64) >> 58];  
}
 
unsigned int lastOne(U64 bitmap)
{
	// this is Eugene Nalimov's bitScanReverse
	// use firstOne if you can, it is faster than lastOne.
	// don't use this if bitmap = 0
 
	int result = 0;
	if (bitmap > 0xFFFFFFFF)
	{
		bitmap >>= 32;
		result = 32;
	}
	if (bitmap > 0xFFFF)
	{
		bitmap >>= 16;
		result += 16;
	}
	if (bitmap > 0xFF)
	{
		bitmap >>= 8;
		result += 8;
	}
	return result + MS1BTABLE[bitmap];
}
 
void displayBitmap(BitMap in)
{
	int i, rank, file;
	char boardc[64];
 
	for (i = 0 ; i < 64 ; i++)
	{
		if (in & BITSET[i]) boardc[i] = '1';
		else boardc[i] = '.';
	}
 
	std::cout << std::endl << "as binary integer:" << std::endl;
      
	for (i = 63 ; i >= 0 ; i--)  std::cout << boardc[i];
	std::cout << std::endl << "  firstOne = " << firstOne(in) << ", lastOne = " << lastOne(in) << ", bitCnt = " << bitCnt(in) << std::endl;
	std::cout << std::endl << std::endl;
 
	if (board.viewRotated)
	{
		std::cout << "   hgfedcba" << std::endl << std::endl;
		for (rank = 1 ; rank <= 8; rank++)
		{
		std::cout << "   ";
		for (file = 8 ; file >= 1; file--)
		{
			std::cout << boardc[BOARDINDEX[file][rank]];
		}
		std::cout << " " << rank << std::endl;
		}
	}
	else
	{
		for (rank = 8 ; rank >= 1; rank--)
		{
			std::cout << " " << rank << " ";
			for (file = 1 ; file <= 8; file++)
			{
				std::cout << boardc[BOARDINDEX[file][rank]];
			}
			std::cout << std::endl;
		}
		std::cout << std::endl << "   abcdefgh" << std::endl;
	}
	std::cout << std::endl;
	return;
}