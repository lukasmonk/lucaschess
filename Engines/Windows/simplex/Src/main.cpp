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

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "Ajedrez.h"
#include "Uci.h"
#define E_INVALIDARG	-1

//#include <io.h>

CPartida Partida;

extern void Seleccion();


/**********************************************************************
** Main: inicializa, reparte el curro y procesa la entrada
*/
int main(int argc, char **argv)
{
	StartThreads(&Partida);
	CUci *protocol = new CUci();	
    protocol->start();
	delete protocol;
	return 0;
}
