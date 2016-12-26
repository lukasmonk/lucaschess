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

union CJugada
{
	unsigned int toint;
	struct 
	{
	public:
		unsigned long f:6;   // casilla inicio
		unsigned long t:6;	 // casilla fin
		unsigned long coronar:3;  // pieza de coronación o pieza comida al paso
		unsigned long alpaso:1;  // casilla donde se come al paso
		unsigned long captura:3;  // casilla donde se come al paso
		unsigned long jaque:1;  // casilla donde se come al paso
		unsigned long pieza:3;  // pieza que se mueve
		unsigned long peso:9;  // peso de la jugada
	} desglose;
	char *ToString()  const;
	inline int ToInt()
	{
		union {
			int i;
			CJugada o;
		} j;
		j.o = *this;
		return j.i;
	};
	inline void Set(int valor)
		{
			union {
				int i;
				CJugada o;
			} j;
			j.i = valor;
			*this = j.o;
		};
	inline friend int operator==(const CJugada &a,const CJugada &b) {
		return (a.desglose.f == b.desglose.f && a.desglose.t == b.desglose.t);// && a.coronar == b.coronar && a.captura == b.captura);
	};
};
