//    Copyright 2010 Antonio Torrecillas Gonzalez
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

//#pragma once
union CJugada;
//class CJugada;
#define MAXHASHLOTE (4 << 20)
#define MAXKILLERS 2
class CHashJugadas
{
public:
	CHashJugadas(void);
	~CHashJugadas(void);
	void Inicializa(void);
	void Push(u64 hash,CJugada &J);
	void Pop(u64 hash,int depth,CJugada &J);
	void SetKiller(int ply,int move);
	int GetKiller(int ply,int num);
	int corte;
	int coloca;
	void SetValue(u64 hash,int value);
	int GetValue(u64 hash);
	void ResetValues();
	int GetJ(u64 hash);
	void SetNoNull(u64 hash);
	char GetNoNull(u64 hash);
	void IncrementaEdad();
private:
	// HASH POSICION MEJOR JUGADA...
	// referencia a las jugadas introducidas
	int pJugadas[MAXHASHLOTE];
	int Killers[100][MAXKILLERS];
	int LastKiller;
	
	int Value[MAXHASHLOTE];
	u64 Verif[MAXHASHLOTE]; // valor para verificar la clave almacenada
	int Edad[MAXHASHLOTE];

	int GetEntrada(u64 hash); // devuelve la entrada libre u ocupada por hash
	int Llamadas[2];
	int EdadActual;
};
#define UBOUND 0x10000
#define LBOUND 0x20000
#define EXACT  0x30000
#define DEPTHMARK 0x100000
#define VALUEMASK 0xffff
#define TRANSDEPTH 1

#define CACHEA(hash,valor,depth,flag) HashJ.SetValue((hash),((valor) & 0x0000ffff)|(flag)|(depth * DEPTHMARK))
