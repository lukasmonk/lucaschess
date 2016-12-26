//    Copyright 2010-2011 Antonio Torrecillas Gonzalez
//
//    This file is part of Simplex.
//
//    Simplex is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Simplex is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Simplex.  If not, see <http://www.gnu.org/licenses/>
//

union CJugada;
#define MAXKILLERS 2

// 16 byte slot * 4 slots/bucket -> 64 bytes -> cache line
typedef struct _Slot {
	u64 Key;
	int value;
	int move;
} SLOT,*PSLOT;

class CHashJugadas
{
private:
	int NumSlots;
	PSLOT TranspositionTable;
	void * TTMem;
public:
	CHashJugadas(void);
	~CHashJugadas(void);
	void SetHashSize(int size);
	void Inicializa(void);
	void SetKiller(int ply,int color,int move);
	int GetKiller(int ply,int color,int num);
	void GetDataCache(u64 hash,int &value,int &move);
	void SetDataCache(u64 hash,int value,int move);
	void IncrementaEdad();
private:
	int Killers[100][2][MAXKILLERS];
	int LastKiller;
	
	int GetSlot(u64 hash); // devuelve la entrada libre u ocupada por hash
	int EdadActual;
};
#define UBOUND 0x10000
#define LBOUND 0x20000
#define EXACT  0x30000
#define DEPTHMARK 0x100000
#define VALUEMASK 0xffff
#define TRANSDEPTH 1
#define AGE 0x10000000
#define CACHE(hash,move,value,depth,flag) HashJ.SetDataCache((hash),((value) & 0x0000ffff)|(flag)|(depth * DEPTHMARK),move)
