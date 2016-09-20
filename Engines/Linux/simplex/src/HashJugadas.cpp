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

#include "Ajedrez.h"

CHashJugadas::CHashJugadas(void)
{
	Inicializa();
}

CHashJugadas::~CHashJugadas(void)
{
}

void CHashJugadas::Inicializa(void)
{
	corte = 0;
	coloca = 0;
//	memset(NoNull,0,sizeof(NoNull));
	memset(pJugadas,0,sizeof(pJugadas));
	memset(Killers,0,sizeof(Killers));
	memset(Verif,0,sizeof(Verif));
	memset(Value,0,sizeof(Verif));
	memset(Edad,0,sizeof(Edad));
	EdadActual = 0;
	Llamadas[0] = 0;
	Llamadas[1] = 0;
	
}

void CHashJugadas::IncrementaEdad()
{
	EdadActual++;
}

void CHashJugadas::Push(u64 hash,CJugada &J)
{
	int entrada = GetEntrada(hash);
	if(entrada)
	{
		pJugadas[entrada] = J.ToInt() & MASCARA_SINPESO;
		Verif[entrada] = (hash);
	}
	return;
}

int CHashJugadas::GetJ(u64 hash)
{
	int entrada = GetEntrada(hash);
	if(entrada)
		return pJugadas[entrada];
	return 0;
}
void CHashJugadas::Pop(u64 hash,int depth,CJugada &J)
{
	int entrada = GetEntrada(hash);
	if(entrada)
	{
	Verif[entrada] = 0;
	pJugadas[entrada] = 0;
	}
}

void CHashJugadas::SetKiller(int ply,int move)
{
	// verificar que no está duplicado
	for(int i = 0;i < MAXKILLERS;i++)
	{
		if(Killers[ply][i] == (move & MASCARA_SINPESO))
			return;
	}
	// es nuevo lo introducimos
	LastKiller++;
	if(LastKiller < 0)
		LastKiller = 0;
	if(LastKiller >= MAXKILLERS)
		LastKiller = 0;
     Killers[ply][LastKiller] = move & MASCARA_SINPESO;
}
int CHashJugadas::GetKiller(int ply,int num)
{
	return Killers[ply][num];
}

void CHashJugadas::ResetValues()
{
	memset(Value,0,sizeof(Value));
}
const int AgeOld = 1;
int CHashJugadas::GetEntrada(u64 hash)
{
	int entrada = hash % MAXHASHLOTE; //& 0xfffff;
	if(Verif[entrada])
	{
		// ya existe una entrada
		if(Verif[entrada] == (hash))
		{
			if((EdadActual != Edad[entrada] ))// > AgeOld)
			{
				Edad[entrada] = 0;
				Verif[entrada] = 0ull;
				Value[entrada] = 0;
				return entrada;
			}
			return entrada;
		}
		else
		{
			// ocupado: si es una entrada vieja la liberamos
			if((EdadActual != Edad[entrada] ))// > AgeOld)
			{
				Edad[entrada] = 0;
				Verif[entrada] = 0ull;
				Value[entrada] = 0;
				return entrada;
			}
			// ocupado utilizamos la posición alternativa
			u64 h = hash;
			do
			{
				h >>= 1;
				entrada = h % MAXHASHLOTE; //& 0xfffff;
				// ocupado: si es una entrada vieja la liberamos
				if((EdadActual != Edad[entrada] ))// > AgeOld)
				{
					Edad[entrada] = 0;
					Verif[entrada] = 0ull;
					Value[entrada] = 0;
					return entrada;
				}
			}
			while(Verif[entrada] != hash && Verif[entrada] != 0 && entrada != 0);
			return entrada;
		}
	}
	else
	{
		return  entrada;
	}
	return 0;
}
void CHashJugadas::SetValue(u64 hash,int value)
{
	int entrada = GetEntrada(hash);
	if(entrada)
	if(((unsigned int )(value&0xfffc0000)) >= (unsigned int)(Value[entrada] & 0xfffc0000))
	{
		// regrabamos si entramos con depth mayor
		Verif[entrada] = (hash);
		Value[entrada] = value;
		Edad[entrada] = EdadActual;
	}
}
int CHashJugadas::GetValue(u64 hash)
{
int entrada = GetEntrada(hash);
	if(Verif[entrada] == hash)
		return Value[entrada];
	return 0;
}
