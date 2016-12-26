//    Copyright 2010-2012 Antonio Torrecillas Gonzalez
//
//    This file is part of Simplex and Rocinante.
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

#include <malloc.h>
#include <memory.h>
#include "HashJugadas.h"

static const int MATE = 32000;

CHashJugadas::CHashJugadas(void)
{
	NumSlots = 32 *1024 * 1024 / (sizeof(SLOT));
	NumSlots--;
	TranspositionTable = 0;
	Inicializa();
}

CHashJugadas::~CHashJugadas(void)
{
}

void CHashJugadas::SetHashSize(int size)
{
	// number of slots.
	NumSlots = (size * 1024 * 1024) / (4 * sizeof(SLOT));
	if(NumSlots < 65536)
		NumSlots = 65536;
	NumSlots--;
	Inicializa();
}

void CHashJugadas::Inicializa(void)
{
	// nuevo sistema
	if(TranspositionTable)
		delete[] TTMem;
	TTMem = (SLOT *) new SLOT[(NumSlots*4)+8]; 
	TranspositionTable = (SLOT *) (((long)TTMem + 63) & ~63); 

	memset(TranspositionTable,0,NumSlots*4*sizeof(SLOT));

	memset(Killers,0,sizeof(Killers));
	EdadActual = 0;
	
}

void CHashJugadas::IncrementaEdad()
{
	EdadActual++;
	if(EdadActual >= 7)
	{
		EdadActual = 1;
	}
}

void CHashJugadas::GetDataCache(u64 hash,int &value,int &move)
{
	int NewEntry = GetSlot(hash);
	if(NewEntry)
	{
		move = TranspositionTable[NewEntry].move;
		if(TranspositionTable[NewEntry].value == MATE)
			value = MATE;
		else
			value = TranspositionTable[NewEntry].value & 0x0fffffff;  // sin la edad
	}
}
void CHashJugadas::SetDataCache(u64 hash,int value,int move)
{
	int NewEntry = GetSlot(hash);
	if(NewEntry)
	{
		TranspositionTable[NewEntry].Key = hash;
		TranspositionTable[NewEntry].move = move;
		if(
			((unsigned int )(value&0x0ffc0000)) >= (unsigned int)(TranspositionTable[NewEntry].value  & 0x0ffc0000)
			|| TranspositionTable[NewEntry].value == MATE
			)
		{
			value |= (EdadActual * AGE);
			TranspositionTable[NewEntry].value = value;
		}
	}
}

void CHashJugadas::SetKiller(int ply,int color,int move)
{
	// verificar que no está duplicado
	for(int i = 0;i < MAXKILLERS;i++)
	{
		if(Killers[ply][color][i] == (move & MASCARA_SINPESO))
			return;
	}
	// es nuevo lo introducimos
	LastKiller++;
	if(LastKiller < 0)
		LastKiller = 0;
	if(LastKiller >= MAXKILLERS)
		LastKiller = 0;
     Killers[ply][color][LastKiller] = move & MASCARA_SINPESO;
}
int CHashJugadas::GetKiller(int ply,int color,int num)
{
	return Killers[ply][color][num];
}

int CHashJugadas::GetSlot(u64 hash)
{
	int i;
	int libre = -1;
	int edad,edadl;
	int entrada = hash % NumSlots;
	edadl = -1;
	entrada *=4; // nos situamos en la linea
	for(i = 0; i < 4;i++,entrada++) // recorremos la linea de cache
	{
		// primero miramos si la encontramos
		if(TranspositionTable[entrada].Key == hash)
		{
			// hemos encontrado una entrada
			// miramos si es vieja -> borrarla
			edad = TranspositionTable[entrada].value >> 28;
			return entrada;
		}
		else
		{
			edad = TranspositionTable[entrada].value >> 28;
			if(edad > edadl && edad != EdadActual)
			{	libre = i;	edadl = edad;	}
		}
	}
	// liberamos la entrada mas vieja
	// replace the oldest one
	if(libre >=0)
	{
		libre += (hash % NumSlots) * 4;
		TranspositionTable[libre].Key = hash;
		TranspositionTable[libre].value = 0;
		TranspositionTable[libre].move = 0;
		return libre; 
	}
	libre = 0;
	return libre; // no slot available
}
