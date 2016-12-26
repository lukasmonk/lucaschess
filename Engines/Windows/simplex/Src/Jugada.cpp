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

#include "Ajedrez.h"

char *CJugada::ToString() const
{
	static char destino[10];
	// rellenar la cadena destino
	memset(destino,0,sizeof(destino));
	destino[0] = (char)('a' + ((desglose.f) & 7)); // COLUMNA
	destino[1] = (char)('1' + ((desglose.f) >> 3)); // FILA
	destino[2] = (char)('a' + ((desglose.t) & 7)); // COLUMNA 
	destino[3] = (char)('1' + ((desglose.t) >> 3)); // FILA
	destino[4] = '\0';
	// falta promociones
	if((desglose.coronar != 0) )//&& (captura == 0))
	{
		static char *l = " pnbrqk ";
		destino[4] = l[desglose.coronar];
		destino[5] = '\0';
	}
	return destino;
}
