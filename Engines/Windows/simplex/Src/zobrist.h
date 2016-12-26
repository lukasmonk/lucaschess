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

extern u64 Zobrist[1500];
#define ZB_ENPASANT 1024
#define ZB_STATUSENROQUE 1090
#define ZB_COLORJUEGAN 1100

inline u64 GetZobKey(int casilla,int color,int pieza);
inline u64 GetZobColor(int colorJuegan);
inline u64 GetZobEnPasant(int en_pasant);
inline u64 GetZobEnroque(int EstadoEnroque);

inline u64 GetZobKey(int casilla,int color,int pieza)
{
    return Zobrist[ casilla * 16 + pieza * 2 + color];
}

inline u64 GetZobColor(int colorJuegan)
{
	return Zobrist[ZB_COLORJUEGAN+colorJuegan];
}

inline u64 GetZobEnPasant(int en_pasant)
{
	return Zobrist[ZB_ENPASANT+en_pasant];
}
inline u64 GetZobEnroque(int EstadoEnroque)
{
	return Zobrist[ZB_STATUSENROQUE+EstadoEnroque];
}
