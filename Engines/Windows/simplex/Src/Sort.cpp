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
#include <assert.h>

extern u64 ataque[8][64]; // casilla pieza // distinguiendo peones blancos y negros

// copiado de los bitboards
void CSort::Init(CDiagrama &Tablero,int EsQuiesce)
{
	// copiamos los bitboards
	BOcupadas[blanco] = Tablero.BOcupadas[blanco];
	BOcupadas[negro] = Tablero.BOcupadas[negro];
	BPiezas[peon][blanco] = Tablero.BPiezas[peon][blanco];
	BPiezas[peon][negro] = Tablero.BPiezas[peon][negro];
	BPiezas[caballo][blanco] = Tablero.BPiezas[caballo][blanco];
	BPiezas[caballo][negro] = Tablero.BPiezas[caballo][negro];
	BPiezas[alfil][blanco] = Tablero.BPiezas[alfil][blanco];
	BPiezas[alfil][negro] = Tablero.BPiezas[alfil][negro];
	BPiezas[torre][blanco] = Tablero.BPiezas[torre][blanco];
	BPiezas[torre][negro] = Tablero.BPiezas[torre][negro];
	BPiezas[dama][blanco] = Tablero.BPiezas[dama][blanco];
	BPiezas[dama][negro] = Tablero.BPiezas[dama][negro];
	BPiezas[rey][blanco] = Tablero.BPiezas[rey][blanco];
	BPiezas[rey][negro] = Tablero.BPiezas[rey][negro];
	PiezasAmenazadas = 0ull;
	OrderQuiesce = EsQuiesce;
#ifdef _DEBUG
	if(!BPiezas[rey][negro] || !BPiezas[rey][blanco])
	{
		// no hay rey !!!!
		int piezas = 2;
	}
#endif

	if(OrderQuiesce)
	{
		HashJugada = 0;
		JHashJugada.Set(0);
		for(int i = 0; i < MAXKILLERS;i++)
			Killer[i] = 0;
	}
	else
	{
		HashJugada = Tablero.HashJugada;
		JHashJugada.Set(HashJugada);
		for(int i = 0; i < MAXKILLERS;i++)
			Killer[i] = Tablero.Killer[i];
	}

	ColorJuegan = Tablero.ColorJuegan;
	EstadoEnroque = Tablero.EstadoEnroque;
	en_pasant = Tablero.en_pasant;
	actual = 0;
	n = 0;
	if(!OrderQuiesce)
	{
		FaseGeneracion = FASE_INICIAL;
	}
	else
	{
			// calculamos los maps
			CalculaMaps();
			if(OrderQuiesce != 3)
			{
				// generamos las capturas y coronaciones.
				JugadasPosibles[actual].Set(0);
				n = GeneraCapturas();
				JugadasPosibles[n].Set(0);
				if(OrderQuiesce == 2)
				{
					PreEsJaque();
					GeneraJaques();
					JugadasPosibles[n].Set(0);
				}
				QuitaHash();
				// aplicamos see
				CalculaPesoQ(&JugadasPosibles[actual],n-actual);
				FaseGeneracion = FASE_CAPBUENAS;
			}
	}
}
const int NoUsaProgresivo= false;
CJugada CSort::GetNext()
{
	if(FaseGeneracion == FASE_INICIAL)
		{
			FaseGeneracion = FASE_HASH;
			if(EsValida(HashJugada))
			{
				JugadasPosibles[actual].Set(HashJugada);
				JugadasPosibles[actual].desglose.peso = PesoHashJugada;
				return JugadasPosibles[actual++];
			}
		}
	if(FaseGeneracion == FASE_HASH)
		{
			// calculamos los maps
			CalculaMaps();
			// generamos las capturas y coronaciones.
			JugadasPosibles[actual].Set(0);
			n = GeneraCapturas();
			JugadasPosibles[n].Set(0);
			QuitaHash();
			// aplicamos see
			CalculaPeso(&JugadasPosibles[actual],n-actual);
			// ordenamos
			OrdenaParcial();
			// devolvemos el primero
			FaseGeneracion = FASE_CAPBUENAS;

			if(actual < n && JugadasPosibles[actual].desglose.peso > PesoCaptura)
			{
				return JugadasPosibles[actual++];
			}
			// generamos el resto de jugadas
			GeneraRestoJugadas();
			JugadasPosibles[n].Set(0);
			QuitaHash();
			JugadasPosibles[n].Set(0);
			// calculamos pesos
			CalculaPeso(&JugadasPosibles[actual],n-actual);
			OrdenaParcial();
			FaseGeneracion = FASE_RESTOJUGADAS;
			if(actual < n)
			{
				return JugadasPosibles[actual++];
			}
		}
		if(FaseGeneracion == FASE_CAPBUENAS)
		{
			// ordenamos
			OrdenaParcial();
			// devolvemos el primero
			if(actual < n && JugadasPosibles[actual].desglose.peso > PesoCaptura)
			{
				return JugadasPosibles[actual++];
			}

			// generamos el resto de jugadas
			GeneraRestoJugadas();
			JugadasPosibles[n].Set(0);
			QuitaHash();
			JugadasPosibles[n].Set(0);
			// calculamos pesos
			CalculaPeso(&JugadasPosibles[actual],n-actual);
			FaseGeneracion = FASE_RESTOJUGADAS;
		}

	
	if(FaseGeneracion == FASE_RESTOJUGADAS)
	{
		if(actual >= n)
		{
			JugadasPosibles[actual].Set(0);
			return JugadasPosibles[actual];
		}
		{
			// ordenamos
			OrdenaParcial();
			return JugadasPosibles[actual++];
		}
	}
	
	// aqui no deberiamos llegar nunca salvo al final
	JugadasPosibles[actual].Set(0);
	return JugadasPosibles[actual];

}

CJugada CSort::GetNextQ()
{
	// ordenamos
	OrdenaParcial();
	// devolvemos el primero
	if(actual < n )
	{
			return JugadasPosibles[actual++];
	}
	// en quiesce solo capturas hemos terminado
	JugadasPosibles[actual].Set(0);
	return JugadasPosibles[actual];
}

void CSort::Rewind()
{
	actual = 0;
}
//
// Verificación si es una jugada posible
//
int CSort::EsValida(int HashJugada)
{
	CJugada J;
	int f,t;
	unsigned int i;
	unsigned int pieza;
	int IsOK = 0;
	if(!HashJugada)
		return 0;
	// from debe haber una pieza del color que toca
	// to corresponde con la captura
	J.Set(HashJugada);
	if(J.desglose.alpaso) return 0;
	if((t = J.desglose.t) == (f = J.desglose.f))
		return 0;
	
	if((pieza = J.desglose.pieza )== 0)
		return 0;

	if(ColorJuegan != color(f))
		return 0;
	if(ColorJuegan == color(t)) // no podemos capturar una pieza propia
		return 0;

	u64 mask = BCasilla(f);

	unsigned int j;
	for(j = peon;j <= rey;j++)
	{
		if(BPiezas[j][ColorJuegan]& mask)
		{
			if(j != pieza)
				return 0;
			IsOK = 1;
			// ahora verificamos que esta pieza puede ir al destino
			if(j == peon)
			{
				if(ColorJuegan == blanco)
				{
					int dif = (J.desglose.t - J.desglose.f);
					if(dif == 8 || dif == 16 || dif == 7 || dif == 9)
						IsOK = 1;
					else
						IsOK = 0;
				}
				else
				{
					int dif = (J.desglose.f - J.desglose.t);
					if(dif == 8 || dif == 16 || dif == 7 || dif == 9)
						IsOK = 1;
					else
						IsOK = 0;
				}
			}
			else
			{
				if(pieza == rey) // puede fallar si el 0-0 es ilegal !!!
				{
					if(J.desglose.f == E1 && (J.desglose.t == G1 || J.desglose.t == C1))
					{
						int cas = J.desglose.t == G1 ? H1:A1;
						if(!(BCasilla(cas) & BPiezas[torre][blanco]))
						{
							return 0;
						}
						// si f1 esta atacada
						CalculaMaps();
						if(EsAtacadaMaps(J.desglose.t == G1 ? F1:D1,blanco)) 
							IsOK = 0;
						else
						{
							// verificar que no hay piezas en la casilla de paso
							if(BCasilla(J.desglose.t == G1 ? F1:D1)& BOcupadas[0])
							{
								IsOK = 0;
							}
							else
							if(BCasilla(J.desglose.t == G1 ? F1:D1)& BOcupadas[1])
							{
								IsOK = 0;
							}
							else
								IsOK = 1;
						}
					}
					else
					if(J.desglose.f == E8 && (J.desglose.t == G8 || J.desglose.t == C8))
					{
						// si f1 esta atacada
						CalculaMaps();
						if(EsAtacadaMaps(J.desglose.t == G8?F8:D8,blanco)) 
							IsOK = 0;
						else
						{
							// verificar que no hay piezas en la casilla de paso
							if(BCasilla(J.desglose.t == G8 ? F8:D8)& BOcupadas[0])
							{
								IsOK = 0;
							}
							else
							if(BCasilla(J.desglose.t == G8 ? F8:D8)& BOcupadas[1])
							{
								IsOK = 0;
							}
							else
								IsOK = 1;
						}
					}
					else
						if(ataque[j][J.desglose.f] & BCasilla(J.desglose.t))
							IsOK = 1;
						else
							IsOK = 0;
				}
				else
				{
				if(ataque[j][J.desglose.f] & BCasilla(J.desglose.t))
					IsOK = 1;
				else
					IsOK = 0;
				}
			}
			break;
		}
	}
	if(!IsOK)
		return 0;
	// ahora verificamos si es torre alfil o dama si no hay cosas entre medio que impidan el movimiento
	i = J.desglose.f -J.desglose.t;
	u64 Ocup = BOcupadas[blanco] | BOcupadas[negro];

	if(pieza != caballo && pieza != rey)
	if((i%8) == 0)
	{
		// movimiento por columna
		if(i > 0)
		{
			for(i=J.desglose.t+8;i< J.desglose.f;i+=8)
			{
				if((Ocup & BCasilla(i)) != 0ull)
					return 0;
			}
		}
		else
		{
			for(i=J.desglose.t-8;i> J.desglose.f;i-=8)
			{
				if((Ocup & BCasilla(i)) != 0ull)
					return 0;
			}
		}
	}
	else
	{
		if((i%7) == 0)
		{
			// movimiento por diagonal
			if(i > 0)
			{
				for(i=J.desglose.t+7;i< J.desglose.f;i+=7)
				{
					if((Ocup & BCasilla(i)) != 0ull)
						return 0;
				}
			}
			else
			{
				for(i=J.desglose.t-7;i> J.desglose.f;i-=7)
				{
					if((Ocup & BCasilla(i)) != 0ull)
						return 0;
				}
			}
		}
		else
		{
			if((i%9) == 0)
			{
				// movimiento por diagonal
				if(i > 0)
				{
					for(i=J.desglose.t+9;i< J.desglose.f;i+=9)
					{
						if((Ocup & BCasilla(i)) != 0ull)
							return 0;
					}
				}
				else
				{
					for(i=J.desglose.t-9;i> J.desglose.f;i-=9)
					{
						if((Ocup & BCasilla(i)) != 0ull)
							return 0;
					}
				}
			}
			else
			{
				// movimiento por diagonal
				if(i > 0)
				{
					for(i=J.desglose.t+1;i< J.desglose.f;i+=1)
					{
						if((Ocup & BCasilla(i)) != 0ull)
							return 0;
					}
				}
				else
				{
					for(i=J.desglose.t-1;i> J.desglose.f;i-=1)
					{
						if((Ocup & BCasilla(i)) != 0ull)
							return 0;
					}
				}
			}
		}
	}

	IsOK  = 0;
	mask = BCasilla(t);
	int c = ColorJuegan^1;
	for(j = peon;j <= rey;j++)
	{
		if(BPiezas[j][ColorJuegan]& mask)
		{
			return 0;
		}
	}
	for(j = peon;j <= rey;j++)
	{
		if(BPiezas[j][c]& mask)
		{
			IsOK = 1;
			break;
		}
	}
	if(IsOK)
	{
		if(j == J.desglose.captura)
			return 1;
		else
			return 0;
	}
	else
	{
		if(J.desglose.captura != ninguna)
			return 0;
	}
	return 1;
}
//
// Esta rutina calcula los bitboards de casillas atacadas por cada pieza
// que hay sobre el tablero
// es un paso común a generar capturas/ jugadas que no son capturas
// evaluar la movilidad de las piezas y muchas funciones ligadas con SEE
// tambien se encargará de llevar la contabilidad de cuantas piezas de cada tipo
// están presentes en el tablero
void CSort::CalculaMaps()
{
	u64 global;
	int c;
	u64 piezas;
	u64 ocupa;
	int casilla;
	int j;

	global = BPiezas[peon][blanco];
	atacadaP[blanco] = (global & 0xfefefefefefefefeull) << 7 | (global & 0x7f7f7f7f7f7f7f7full)<< 9 ;
	global = BPiezas[peon][negro];
	atacadaP[negro] = (global & 0x7f7f7f7f7f7f7f7full) >> 7 | (global & 0xfefefefefefefefeull) >> 9;

	// por cada color
  for(c = 0; c < 2;c++)
  {
	// de rey
	piezas = BPiezas[rey][c];
#ifdef _DEBUG
	if(!piezas)
	{
		// no hay rey !!!!
		piezas = piezas;
	}
#endif
	// buscamos la casilla donde se ubica el rey
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// en casilla tenemos lo buscado
		Maps[c][rey][0] = ataque[rey][casilla];
		PosP[c][rey][0] = casilla;
		PosReyes[c] = casilla;
	}
  }

  PreEsJaque();
	ocupa = BOcupadas[blanco] |BOcupadas[negro];

  for(c = 0; c < 2;c++)
  {
	  // map global de rey
	global = Maps[c][rey][0];
	  // primero caballos
	piezas = BPiezas[caballo][c];
	j = 0;
	Maps[c][caballo][j] = 0ull;
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// en casilla tenemos lo buscado
		Maps[c][caballo][j] = ataque[caballo][casilla];
		PosP[c][caballo][j] = casilla;
		global |= Maps[c][caballo][j];

		j++;
	}
	Maps[c][caballo][j] = 0ull;
	Caballos[c] = j;
	// Torres por filas y columnas
	piezas = BPiezas[torre][c];
	j = 0;
	Maps[c][torre][j] = 0ull;
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// en casilla tenemos lo buscado
		Maps[c][torre][j]= rankAttacks(ocupa,casilla) |(fileAttacks(ocupa,casilla));
		PosP[c][torre][j] = casilla;
		global |= Maps[c][torre][j];
		j++;
	}
	Maps[c][torre][j] = 0ull;
	Torres[c] = j;
	// Alfiles por diagonales
	piezas = BPiezas[alfil][c];
	j = 0;
	Maps[c][alfil][j] = 0ull;
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// en casilla tenemos lo buscado
		Maps[c][alfil][j] = (diagonalAttacks(ocupa,casilla)) | (antiDiagAttacks(ocupa,casilla));
		PosP[c][alfil][j] = casilla;
		global |= Maps[c][alfil][j];
		j++;
	}
	Maps[c][alfil][j] = 0ull;
	Alfiles[c] = j;
	
	// damas
	piezas = BPiezas[dama][c];
	j = 0;
	Maps[c][dama][j] = 0ull;
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// en casilla tenemos lo buscado
		Maps[c][dama][j] = (diagonalAttacks(ocupa,casilla)) | (antiDiagAttacks(ocupa,casilla))
			| (rankAttacks(ocupa,casilla)) |(fileAttacks(ocupa,casilla));
		PosP[c][dama][j] = casilla;
		global |= Maps[c][dama][j];
		j++;
	}
	Maps[c][dama][j] = 0ull;
	Damas[c] = j;
	// salvamos el mapa global
	Maps[c][ninguna][0] = global;
  }
  CalculaAmenazadas();
}


///////////////////////////////////////////////////////////
// Genera las capturas usando bitboard
int CSort::GeneraCapturas()
{
	register u64 piezas;
	register CJugada *Jaux; // puntero a las jugadas para ir rellenando
	u64 AtC;
	int casilla;
	int t;
	int c = ColorJuegan^1;
	int retorno = actual;
	int i;
u64 BReyC = BPiezas[rey][c];

	Jaux = &JugadasPosibles[actual];
	// primero capturas de caballo
	for(i = 0; i < Caballos[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][caballo][i] & BOcupadas[c];
		casilla = PosP[ColorJuegan][caballo][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | caballo << DESP_PIEZA);
			if(ataque[caballo][t] & BReyC)
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// de rey
	casilla = PosReyes[ColorJuegan];
	// el rey es muy delicado y no puede capturar una pieza que está defendida!!
	AtC = Maps[ColorJuegan][rey][0] & BOcupadas[c] & ~Maps[c][ninguna][0];
	while(AtC)
	{
		t = bitScanAndReset(AtC);
		Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | rey << DESP_PIEZA);
//		Jaux->peso = rey;
		retorno++;
		Jaux++;
	}

	// de peon
	if(ColorJuegan == blanco)
	{
		piezas = BPiezas[peon][blanco];
		// buscamos la casilla donde se ubica el rey
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
			// en casilla tenemos lo buscado
			AtC = ataque[peonn][casilla] & BOcupadas[negro];
			while(AtC)
			{
				t = bitScanAndReset(AtC);
				if(FILA(t) == 7)
				{
Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| dama << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
								retorno++;
								Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| torre << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC ) & BCasilla(t))
				Jaux->desglose.jaque = 1;
								retorno++;
								Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| alfil << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
								retorno++;
								Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| caballo << DESP_CORONAR | peon << DESP_PIEZA);
		if(ataque[caballo][t] & BReyC)
			Jaux->desglose.jaque = 1;
								retorno++;
								Jaux++;
				}
				else
				{
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | peon << DESP_PIEZA);
		if(ataque[peon][t] & BReyC)
			Jaux->desglose.jaque = 1;
								retorno++;
								Jaux++;
				}
			}
		}
		// capturas al paso
		if(FILA(en_pasant) == 5)
		{
			if(COLUMNA(en_pasant))
			{
				casilla = en_pasant-9;
				if(BPiezas[peon][blanco] & BCasilla(casilla))
				{
					Jaux->Set(casilla | en_pasant << DESP_TO | peon << DESP_PIEZA | peon << DESP_CAPTURA);
					Jaux->desglose.alpaso = 1;
		if(ataque[peon][en_pasant] & BReyC)
			Jaux->desglose.jaque = 1;
					Jaux++;
					retorno++;
				}
			}
			if(COLUMNA(en_pasant)<7)
			{
				casilla = en_pasant-7;
				if(BPiezas[peon][blanco] & BCasilla(casilla))
				{
			Jaux->Set(casilla | en_pasant << DESP_TO | peon << DESP_PIEZA | peon << DESP_CAPTURA);
					Jaux->desglose.alpaso = 1;
		if(ataque[peon][en_pasant] & BReyC)
			Jaux->desglose.jaque = 1;
					Jaux++;
					retorno++;
				}
			}
		}
		// coronaciones del blanco
		piezas = BPiezas[peon][blanco] & 0x00ff000000000000ull;
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
				if(board(casilla+8) == ninguna)
				{
					t = casilla +8;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| dama << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| torre << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC ) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| alfil << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| caballo << DESP_CORONAR | peon << DESP_PIEZA);
		if(ataque[caballo][t] & BReyC)
			Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
		}
	}
	else	// juegan negras
	{
		piezas = BPiezas[peon][negro];
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
			// en casilla tenemos lo buscado
			AtC = ataque[peon][casilla] & BOcupadas[blanco];
			while(AtC)
			{
				t = bitScanAndReset(AtC);
				if(FILA(t) == 0)
				{
Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| dama << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| torre << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC ) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| alfil << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA| caballo << DESP_CORONAR | peon << DESP_PIEZA);
		if(ataque[caballo][t] & BReyC)
			Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
				else
				{
					Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | peon << DESP_PIEZA);
		if(ataque[peonn][t] & BReyC)
			Jaux->desglose.jaque = 1;
					Jaux++;
					retorno++;
				}
			}
		}
		// capturas al paso
		if(FILA(en_pasant) == 2)
		{
			if(COLUMNA(en_pasant))
			{
				casilla = en_pasant+7;
				if(BPiezas[peon][negro] & (1ull << casilla))
				{
					Jaux->Set(casilla | en_pasant << DESP_TO | peon << DESP_PIEZA | peon << DESP_CAPTURA);
					Jaux->desglose.alpaso = 1;
		if(ataque[peonn][en_pasant] & BReyC)
			Jaux->desglose.jaque = 1;
					Jaux++;
					retorno++;
				}
			}
			if(COLUMNA(en_pasant)<7)
			{
				casilla = en_pasant+9;
				if(BPiezas[peon][negro] & (1ull << casilla))
				{
					Jaux->Set(casilla | en_pasant << DESP_TO | peon << DESP_PIEZA | peon << DESP_CAPTURA);
					Jaux->desglose.alpaso = 1;
		if(ataque[peonn][en_pasant] & BReyC)
			Jaux->desglose.jaque = 1;
					Jaux++;
					retorno++;
				}
			}
		}
		// coronaciones del negro
		piezas = BPiezas[peon][negro] & 0x000000000000ff00ull;
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
			// en casilla tenemos lo buscado
			if(board(casilla-8) == ninguna)
			{
				t = casilla -8;
				Jaux->Set(casilla | t << DESP_TO | board(t) << DESP_CAPTURA| dama << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
				retorno++;
				Jaux++;
				Jaux->Set(casilla | t << DESP_TO | board(t) << DESP_CAPTURA| torre << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtFC ) & BCasilla(t))
				Jaux->desglose.jaque = 1;
				retorno++;
				Jaux++;
				Jaux->Set(casilla | t << DESP_TO | board(t) << DESP_CAPTURA| alfil << DESP_CORONAR | peon << DESP_PIEZA);
			if((AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
				retorno++;
				Jaux++;
				Jaux->Set(casilla | t << DESP_TO | board(t) << DESP_CAPTURA| caballo << DESP_CORONAR | peon << DESP_PIEZA);
		if(ataque[caballo][t] & BReyC)
			Jaux->desglose.jaque = 1;
				retorno++;
				Jaux++;
			}
		}
	}
	// capturas por damas
	for(i = 0; i < Damas[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][dama][i] & BOcupadas[c];
		casilla = PosP[ColorJuegan][dama][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | board(t) << DESP_CAPTURA | dama << DESP_PIEZA);
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// capturas por Torres
	for(i = 0; i < Torres[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][torre][i] & BOcupadas[c];
		casilla = PosP[ColorJuegan][torre][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | torre << DESP_PIEZA);
			if((AtFC) & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}

	// capturas por Alfiles
	for(i = 0; i < Alfiles[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][alfil][i] & BOcupadas[c];
		casilla = PosP[ColorJuegan][alfil][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO |board(t) << DESP_CAPTURA | alfil << DESP_PIEZA);
			if(AtDiag & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	return retorno;
}

// no se contemplan los enroques que dan jaque
// ni las descubiertas
// ni jaques de peon.
int CSort::GeneraJaques()
{
	extern int MSB(u64 a);
	register CJugada *Jaux; // puntero a las jugadas para ir rellenando
	u64 AtC;
	int casilla;
	int t;
	int retorno = 0;
	u64 ocupa;
	u64 aux;
	int i;
u64 BReyC = 0x0ull;

	BReyC = BPiezas[rey][ColorJuegan^1];

	ocupa = BOcupadas[blanco] |BOcupadas[negro];
	ocupa  = ~ocupa; // las casillas libres
	aux = ocupa;

	Jaux = &JugadasPosibles[n];
	// primero no capturas de caballo
	for(i = 0; i < Caballos[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][caballo][i] & ataque[caballo][PosReyes[ColorJuegan^1]] 
		& ocupa;
		casilla = PosP[ColorJuegan][caballo][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | caballo << DESP_PIEZA);
			Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Alfil
	for(i = 0; i < Alfiles[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][alfil][i] & aux & AtDiag;
		casilla = PosP[ColorJuegan][alfil][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | alfil << DESP_PIEZA);
			Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Torre
	for(i = 0; i < Torres[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][torre][i] & aux & AtFC;
		casilla = PosP[ColorJuegan][torre][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | torre << DESP_PIEZA);
			Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Damas
	for(i = 0; i < Damas[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][dama][i] & aux & (AtDiag|AtFC);
		casilla = PosP[ColorJuegan][dama][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | dama << DESP_PIEZA);
			Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	n += retorno;
	return retorno;
}


int CSort::GeneraRestoJugadas()
{
	extern int MSB(u64 a);
	register u64 piezas;
	register CJugada *Jaux; // puntero a las jugadas para ir rellenando
	u64 AtC;
	int casilla;
	int t;
	int retorno = 0;
	u64 ocupa;
	u64 aux;
	int i;
u64 BReyC = 0x0ull;

	BReyC = BPiezas[rey][ColorJuegan^1];

	ocupa = BOcupadas[blanco] |BOcupadas[negro];
	ocupa  = ~ocupa; // las casillas libres
	aux = ocupa;

	Jaux = &JugadasPosibles[n];
	// enroques 
	if(!EstoyEnJaque())
	{
		if(!(EstadoEnroque & MRB)&& ColorJuegan == blanco)
		{
			// enroque corto 0-0
			if(!(EstadoEnroque & MTRB))
			{
				// no hay piezas enmedio
				if(board(5) == ninguna && board(6) == ninguna && board(7) == torre && color(7) == blanco)
				{
					// hay alguna pieza que ataque el escaque de paso
					if(EsAtacadaMaps(4,blanco)|| EsAtacadaMaps(5,blanco) )
					{
					}
					else
					{
						// 0-0 ok
						Jaux->Set(4 | 6 << DESP_TO | rey << DESP_PIEZA);
//		Jaux->desglose.peso = rey;
						Jaux++;
						retorno++;
					}
				}

			}
			// probar enroque largo
			if(!(EstadoEnroque & MTDB))
			{
				// no hay piezas enmedio
				if(board(2) == ninguna && board(3) == ninguna && board(1) == ninguna && board(0) == torre && color(0) == blanco)
				{
					// hay alguna pieza que ataque el escaque de paso
					if(EsAtacadaMaps(2,blanco) || EsAtacadaMaps(3,blanco) )
					{
					}
					else
					{
						// 0-0 ok
						Jaux->Set(4 | 2 << DESP_TO | rey << DESP_PIEZA);
//		Jaux->desglose.peso = rey;
						Jaux++;
						retorno++;
					}
				}
			}

			// fin 0-0-0
		}
		// negras
		if(!(EstadoEnroque & MRN)&& ColorJuegan == negro)
		{
			// enroque corto 0-0
			if(!(EstadoEnroque & MTRN))
			{
				// no hay piezas enmedio
				if(board(61) == ninguna && board(62) == ninguna && board(63) == torre && color(63) == negro)
				{
					if(EsAtacadaMaps(61,negro) || EsAtacadaMaps(62,negro) )
					{
					}
					else
					{
						// 0-0 ok
						Jaux->Set(60 | 62 << DESP_TO | rey << DESP_PIEZA);
//		Jaux->desglose.peso = rey;
						Jaux++;
						retorno++;
					}
				}
			}
			// probar enroque largo
			if(!(EstadoEnroque & MTDN))
			{
				// no hay piezas enmedio
				if(board(59) == ninguna && board(58) == ninguna && board(57) == ninguna && board(56) == torre && color(56) == negro)
				{
					if(EsAtacadaMaps(58,negro) || EsAtacadaMaps(59,negro) )
					{
					}
					else
					{
						// 0-0 ok
						Jaux->Set(60 | 58 << DESP_TO | rey << DESP_PIEZA);
//		Jaux->desglose.peso = rey;
						Jaux++;
						retorno++;
					}
				}
			}
			// fin 0-0-0
		}
	}

	// primero no capturas de caballo
	for(i = 0; i < Caballos[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][caballo][i] & ocupa;
		casilla = PosP[ColorJuegan][caballo][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | caballo << DESP_PIEZA);
//		Jaux->desglose.peso = caballo;
			if(ataque[caballo][t] & BReyC)
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Alfil
	for(i = 0; i < Alfiles[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][alfil][i] & aux;
		casilla = PosP[ColorJuegan][alfil][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | alfil << DESP_PIEZA);
//		Jaux->desglose.peso = alfil;
			if((AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Torre
	for(i = 0; i < Torres[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][torre][i] & aux;
		casilla = PosP[ColorJuegan][torre][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | torre << DESP_PIEZA);
//		Jaux->desglose.peso = torre;
			if((AtFC) & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}
	// no capturas de Damas
	for(i = 0; i < Damas[ColorJuegan];i++)
	{
		AtC = Maps[ColorJuegan][dama][i] & aux;
		casilla = PosP[ColorJuegan][dama][i] ;
		while(AtC)
		{
			t = bitScanAndReset(AtC);
			Jaux->Set(casilla | t << DESP_TO | dama << DESP_PIEZA);
//		Jaux->desglose.peso = dama;
			if((AtFC |AtDiag) & BCasilla(t))
				Jaux->desglose.jaque = 1;
			retorno++;
			Jaux++;
		}
	}

	// de peon
	if(ColorJuegan == blanco)
	{
		piezas = BPiezas[peon][blanco];
		piezas &= 0x0000ffffffffff00ull;// descartamos peones en 7ª
		// buscamos la casilla donde se ubica el rey
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
			{
				// en casilla tenemos lo buscado
				if( board(casilla+8) == ninguna)
				{
					Jaux->Set(casilla | (casilla+8) << DESP_TO | peon << DESP_PIEZA);
//		Jaux->desglose.peso = peon;
			if(ataque[peon][casilla+8] & BReyC)
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
				if(FILA(casilla) == 1 && board(casilla+8) == ninguna && board(casilla+16) == ninguna)
				{
					Jaux->Set(casilla | (casilla+16) << DESP_TO | peon << DESP_PIEZA);
//		Jaux->desglose.peso = peon;
			if(ataque[peon][casilla+16] & BReyC)
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
			}
		}
	}
	else	// juegan negras
	{
		piezas = BPiezas[peon][negro] & 0x00ffffffffff0000ull; // descartamos peones en 7ª
		// buscamos la casilla donde se ubica el rey
		while(piezas)
		{
			casilla = bitScanAndReset(piezas);
			{
				if(casilla > 8 && board(casilla-8) == ninguna)
				{
					Jaux->Set(casilla | (casilla-8) << DESP_TO | peon << DESP_PIEZA);
//		Jaux->desglose.peso = peon;
			if(ataque[peonn][casilla-8] & BReyC)
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
				if(FILA(casilla) == 6 && board(casilla-8) == ninguna && board(casilla-16) == ninguna)
				{
					Jaux->Set(casilla | (casilla-16) << DESP_TO | peon << DESP_PIEZA);
//		Jaux->desglose.peso = peon;
			if(ataque[peonn][casilla-16] & BReyC)
				Jaux->desglose.jaque = 1;
					retorno++;
					Jaux++;
				}
			}
		}
		// capturas al paso
	}

	// no capturas de rey donde no este amenazado
	AtC = Maps[ColorJuegan][rey][0] & aux & ~Maps[ColorJuegan^1][ninguna][0];
	casilla = PosReyes[ColorJuegan];
	while(AtC)
	{
		t = bitScanAndReset(AtC);
		Jaux->Set(casilla | t << DESP_TO | rey << DESP_PIEZA);
//		Jaux->desglose.peso = rey;
		retorno++;
		Jaux++;
	}


	// actualizamos PosReyes
	n += retorno;
	return retorno;
}

int CSort::board(int t)
{
	u64 mask = BCasilla(t);
	// primero miramos si está libre
	if(!(mask & (BOcupadas[blanco] | BOcupadas[negro])))
		return ninguna;
	for(int j = peon;j <= rey;j++)
	{
		if(BPiezas[j][blanco]& mask)
		{
			return j;
		}
		if(BPiezas[j][negro]& mask)
		{
			return j;
		}
	}
	return ninguna;
}
int CSort::color(int t)
{
	u64 mask = BCasilla(t);

	for(int i = 0; i < 2;i++)
	for(int j = peon;j <= rey;j++)
	{
		if(BPiezas[j][i]& mask)
		{
			return i;
		}
	}
	return neutro;
}

void CSort::CalculaPeso(CJugada  Jugadas[MAXJUGADAS], int tope)
{
	int i;
	CJugada *aux;
	assert(tope < MAXJUGADAS);
	aux = &Jugadas[0];
	int oa = actual;
	for(i=actual;i < tope;i++,aux++)
	{
		actual = i;
		EvaluaPeso(*aux);
	}
	actual = oa;
}
void CSort::CalculaPesoQ(CJugada  Jugadas[MAXJUGADAS], int tope)
{
	int i;
	CJugada *aux;
	assert(tope < MAXJUGADAS);
	aux = &Jugadas[0];
	int oa = actual;
	for(i=0;i < tope;i++,aux++)
	{
		actual = i;
		EvaluaPesoQ(*aux);
	}
	actual = oa;
}
void CSort::PreEsJaque()
{
	u64 ocupa;
	int casilla;
	PosReyContrario = casilla = PosReyes[ColorJuegan^1];
#ifdef _DEBUG
	if(casilla < 0)
	{
		casilla = casilla;
	}
#endif
	ReyContrario = BCasilla(casilla);
	ocupa = BOcupadas[blanco] |BOcupadas[negro];
	AtFC = rankAttacks(ocupa,casilla) |(fileAttacks(ocupa,casilla) );
	AtDiag = (diagonalAttacks(ocupa,casilla)) | (antiDiagAttacks(ocupa,casilla));
}
int CSort::EsJaque(CJugada &j)
{
	// mira si esta jugada es jaque
	int casilla;
	casilla = j.desglose.t;
	u64 aux;
	// primero miramos si podemos descartar con facilidad
	aux = BCasilla(casilla);
	if(!(aux & (AtDiag |AtFC |ataque[caballo][casilla])))
		return 0;
	// puede ser un jaque veamos con más detalle
	char p = (char)board(j.desglose.f);
	if(j.desglose.coronar)
		p = (char)j.desglose.coronar;
	switch(p)
	{
	case peon:
		{
			int dif = casilla - PosReyContrario;
			if(dif == 7 || dif == 9 || dif == -7 || dif == -9)
			{
				if(ColorJuegan == blanco && dif <0)
				{
					int col = COLUMNA(casilla);
					if(col==0 && (dif == -7 || dif == 9))
						return 0;
					if(col==7 && (dif == 7 || dif == -9))
						return 0;
					return 1;
				}
				if(ColorJuegan == negro && dif > 0)
				{
					int col = COLUMNA(casilla);
					if(col==0 && (dif == -7 || dif == 9))
						return 0;
					if(col==7 && (dif == 7 || dif == -9))
						return 0;
					return 1;
				}
			}
			return 0;
		}
		break;
	case caballo:
		if(ataque[caballo][casilla] & ReyContrario)
			return 1;
		return 0;
		break;
	case alfil:
		aux = BCasilla(casilla);
		if(AtDiag & aux)
			return 1;
		else
			return 0;
	case torre:
		aux = BCasilla(casilla);
		if(AtFC  & aux )
			return 1;
		return 0;
		break;
	case dama:
		aux = BCasilla(casilla);
		if(AtDiag & aux)
			return 1;
		if(AtFC  & aux )
			return 1;
		return 0;
		break;
	default:
		return 0;
	}
	return 0;
}

void CSort::EvaluaPeso(CJugada &J)
{
	int peso = 0;
	int captura = 0;
	int Jug = J.ToInt() & MASCARA_SINPESO;
	if(Jug == HashJugada )
	{
		J.desglose.peso = PesoHashJugada;
		return;
	}
	for(int k = 0; k < MAXKILLERS;k++)
	{
		if(Jug  == Killer[k] )
		{
			J.desglose.peso = PesoKiller;
			return;
		}
	}
	captura = J.desglose.captura  + J.desglose.coronar;
	if(captura)
	{
		peso = PesoCaptura + captura;
		if( EsCapturaBuena(J))
		{
			peso += PesoCapturaBuena;
		}
		else
		{
			if(ValorPiezas[PIEZA(J)] > ValorPiezas[captura])
			{
				// captura mala
				peso = PesoCapturaMala+captura;
			}
		}
	}
	else // es captura
	{
		{   // No es enroque
			// si es una pieza amenazada o no
			if(BCasilla(J.desglose.f) & PiezasAmenazadas && EsSegura(J))
			{
				peso = PesoAmenazada+J.desglose.pieza;
			}
			else
			{
				peso = JugadaNormal;
			}
		}
	}
	if(peso == 0)
		peso =JugadaNormal;
	J.desglose.peso = peso;
}

void CSort::EvaluaPesoQ(CJugada &J)
{
	int peso1 = 0;
	int captura = 0;

	captura = J.desglose.captura + J.desglose.coronar - J.desglose.pieza;
	peso1 = PesoCaptura + captura;
	if( EsCapturaBuena(J))
	{
		peso1 += PesoCapturaBuena;
	}
	else
	{
		if(ValorPiezas[PIEZA(J)] > ValorPiezas[captura])
		{
			// captura mala
			peso1 = PesoCapturaMala+captura;
		}
	}
	J.desglose.peso = peso1;
}

int CSort::EsCapturaBuena(CJugada &J)
{
	if(J.desglose.coronar != ninguna)
	{
		if(J.desglose.coronar == dama && !EsAtacadaMaps(J.desglose.t,ColorJuegan^1))
			return 1;
	}
	else
	if(J.desglose.captura == ninguna)
		return 0;

	unsigned int pieza = (unsigned)PIEZA(J);
	unsigned int captura = J.desglose.captura;
	//	if((unsigned)board(J.f) < (unsigned)J.captura )
	if(pieza < captura )
	{
		if((!(pieza == caballo && captura == alfil)) &&
			(!(pieza == alfil && captura == caballo)) 
			)
			return 1;
	}
	// es una captura entre material equivalente
	// miramos si tenemos control de la casilla destino
	int t = J.desglose.t;
	if(!EsAtacadaMaps(t,color(t)^1))
		return 1;

	// resto de casos
	// pieza de mismo valor pero con mayor atacantes que defensores
	int v = ValorPiezas[pieza];
	int cap = ValorPiezas[captura] ;

	if(cap == v )
	{
		int c = Cuenta(t);
		if(c <= 0)
			return 0;
		return 1;
	}
	if ( cap < v
		&& MenorDefensor(t) >= v
	) 
	{
		int c = Cuenta(t);
		if(c <= 0)
			return 0;
		return 1;
	}
	return 0;
}

// Contamos las piezas de color "color" que atacan/defienden la casilla "casilla"
int CSort::CuentaC(int casilla,int color1)
{
	int cuenta = 0;
	u64 AtC = 0;
	int c = color1;
	int i;
	// ahora piezas deslizantes
	// hay que tener en cuenta las sombras y tal

	AtC = BCasilla(casilla);
	for(i = 0; i < Caballos[color1];i++)
	{
		if(Maps[color1][caballo][i] & AtC)
			cuenta++;
	}
	for(i = 0; i < Alfiles[color1];i++)
	{
		if(Maps[color1][alfil][i] & AtC)
		{
   			cuenta++;
			// buscar atacante oculto una dama por ejemplo
			for(int j = 0; j < Damas[color1];j++)
			{
				if(Maps[color1][dama][j] & BCasilla(PosP[color1][alfil][i]) 
					&& ataque[dama][PosP[color1][dama][j]] & AtC
					)
					cuenta++;
			}
		}
	}
	for(i = 0; i < Torres[color1];i++)
	{
		if(Maps[color1][torre][i] & AtC)
		{
			cuenta++;
			// buscar atacante oculto una dama por ejemplo u otra torre
			for(int j = 0; j < Damas[color1];j++)
			{
				if(Maps[color1][dama][j] & BCasilla(PosP[color1][torre][i]) 
					&& ataque[dama][PosP[color1][dama][j]] & AtC
					)
					cuenta++;
			}
			for(int j = 0; j < Torres[color1];j++)
			{
				if(Maps[color1][torre][j] & BCasilla(PosP[color1][torre][i]) 
					&& ataque[torre][PosP[color1][torre][j]] & AtC
					)
					cuenta++;
			}
		}
	}
	for(i = 0; i < Damas[color1];i++)
	{
		if(Maps[color1][dama][i] & AtC)
		{
			cuenta++;
			// buscar atacante oculto una alfil por ejemplo 
			for(int j = 0; j < Alfiles[color1];j++)
			{
				if(Maps[color1][alfil][j] & BCasilla(PosP[color1][dama][i]) 
					&& ataque[alfil][PosP[color1][alfil][j]] & AtC
					)
					cuenta++;
			}
			// buscar atacante oculto una torre por ejemplo 
			for(int j = 0; j < Torres[color1];j++)
			{
				if(Maps[color1][torre][j] & BCasilla(PosP[color1][dama][i]) 
					&& ataque[torre][PosP[color1][torre][j]] & AtC
					)
					cuenta++;
			}
		}
	}

	if(Maps[color1][rey][0] & AtC)
		cuenta++;
	if(c == blanco)
	{
		AtC = (AtC & 0x7f7f7f7f7f7f7f7full) >> 7 | (AtC & 0xfefefefefefefefeull)>> 9;
		cuenta += popCount(AtC & BPiezas[peon][blanco]);
	}
	else
	{
		AtC = (AtC & 0xfefefefefefefefeull) << 7 | (AtC & 0x7f7f7f7f7f7f7f7full) << 9;
		cuenta += popCount(AtC & BPiezas[peon][negro]);
	}
	return cuenta;
}
int CSort::MenorDefensor(int casilla)
{
	u64 AtC = 0;
	int c = ColorJuegan^1;
//	if(c == blanco)
	if(ColorJuegan == blanco)
	{
		AtC = ataque[peonn][casilla] & BPiezas[peon][negro];
	}
	else
	{
		AtC = ataque[peon][casilla] & BPiezas[peon][blanco];
	}
	if(AtC)
		return ValorPiezas[peon];

	AtC = BCasilla(casilla);
	int i;
	for(i=0; i < Caballos[c];i++)
	{
		if(AtC & Maps[c][caballo][i])
			return ValorPiezas[caballo];
	}
	for(i=0; i < Alfiles[c];i++)
	{
		if(AtC & Maps[c][alfil][i])
			return ValorPiezas[alfil];
	}
	for(i=0; i < Torres[c];i++)
	{
		if(AtC & Maps[c][torre][i])
			return ValorPiezas[torre];
	}
	for(i=0; i < Damas[c];i++)
	{
		if(AtC & Maps[c][dama][i])
			return ValorPiezas[dama];
	}
	// si defiende el rey en minoria no defiende
	return 30000;
}

int CSort::Cuenta(int casilla)
{
	// cuenta de piezas atacantes y defendientes
	int cuenta = 0;
	// contamos las propias
	cuenta += CuentaC(casilla,ColorJuegan);
	cuenta -= CuentaC(casilla,ColorJuegan^1);
	return cuenta;
}

int CSort::EsAtacadaMaps(int casilla,int bando)
{
	// determina si el bando contrario al 'bando' ataca la casilla definida
	u64 AtC = 0;
	int c = bando^1;
	AtC = BCasilla(casilla);
	if(Maps[c][ninguna][0] & AtC)
		return 1;
	if(bando == blanco)
	{
		// Queremos aqui saber si casilla esta atacada por un peon negro
		if(atacadaP[negro] & BCasilla(casilla))
			return 1;
	}
	else
	{
		// Queremos aqui saber si casilla esta atacada por un peon blanco
		if(atacadaP[blanco] & BCasilla(casilla))
			return 1;
	}
	return 0;
}
int compare( const void *arg1, const void *arg2 )
{
   /* Compare all of both strings: */
   return ( *( unsigned int* ) arg1 > *( unsigned int* )arg2 );
}

void CSort::OrdenaParcial()
{
	register int i,iPivote;
	int cual;
	int peso_ref = 0; 
	int peso1;
	cual = actual;
	if(actual > 4)
		return;
	// ahora ordenamos solamente
	for(i=actual;i < n;i++)
	{
		peso1 = JugadasPosibles[i].desglose.peso;
		if(peso1 > peso_ref)
		{
			// colocar al principio
			cual = i;
			peso_ref = peso1;
		}
	}

	if(cual != actual)
	{
		// mover
		// intercambiar
		iPivote = JugadasPosibles[actual].toint;//.ToInt();
		JugadasPosibles[actual].Set(JugadasPosibles[cual].toint);//.ToInt());
		JugadasPosibles[cual].Set(iPivote);
	}
}

int CSort::EstoyEnJaque()
{
	// ver si esta atacado
	return EsAtacadaMaps(PosReyes[ColorJuegan],ColorJuegan);
}


void CSort::QuitaHash()
{
	int i,j;
	if(!HashJugada)
		return;
	for(i = actual; i < n;i++)
	{
		if((JugadasPosibles[i].ToInt()&0xffff) == (HashJugada & 0xffff))
		{
			// quitarla de la lista de jugadas pendientes.
			for(j = i+1;j < n;j++)
			{
				JugadasPosibles[j-1].Set(JugadasPosibles[j].ToInt());
			}
			JugadasPosibles[j].Set(0);
			n--;
			JugadasPosibles[n].Set(0);
			break;
		}
	}
}
void CSort::CalculaAmenazadas()
{
	int c = ColorJuegan^1;
	u64 aux,aux1;
	int i;

	PiezasAmenazadas = 0ull;
	// miramos si nos amenazan algo
	aux = BOcupadas[ColorJuegan] & Maps[c][ninguna][0];
	if(!aux)
		return;

	// piezas indefensas
	PiezasAmenazadas |= aux & ~Maps[ColorJuegan][ninguna][0];
	// piezas y peones amenazados por peones contrarios
	PiezasAmenazadas |= BOcupadas[ColorJuegan] & atacadaP[c];
	// Torres y damas atacadas por caballos y alfiles
	aux1 = BPiezas[torre][ColorJuegan] | BPiezas[dama][ColorJuegan];
	if(aux & aux1) // hay alguna atacada
	{
		for(i = 0; i < Caballos[c];i++)
		{
			PiezasAmenazadas |= aux & Maps[c][caballo][i];
		}
		for(i = 0; i < Alfiles[c];i++)
		{
			PiezasAmenazadas |= aux & Maps[c][alfil][i];
		}
	}
	// Damas atacadas por torres
	aux1 = BPiezas[dama][ColorJuegan];
	if(aux1 & aux)
	{
		for(i=0; i < Torres[c];i++)
		{
			PiezasAmenazadas |= aux & Maps[c][torre][i];
		}
	}
}

int CSort::EsDoble(CJugada J)
{
	u64 AtC,piezas;
	int c = ColorJuegan^1;
	if(board(J.desglose.f) == caballo)
	{
		AtC = ataque[caballo][J.desglose.t];
	// de momento simplificando tomamos reyes torres y damas
		piezas = BPiezas[rey][c]|BPiezas[torre][c]|BPiezas[dama][c];
		AtC &= piezas;
		AtC &= AtC - 1; // clear the least significant bit set
		if(AtC)
			return 1;
	}
	if(board(J.desglose.f) == peon)
	{
		// ver si desde aquí se atacan 2
		if(c == blanco)
		{
			if(board(J.desglose.t-7) > peon && board(J.desglose.t-9) > peon
				&& color(J.desglose.t-7) == blanco && color(J.desglose.t-9) == blanco
				)
				return 1;
		}
		else
		{
			if(board(J.desglose.t+7) > peon && board(J.desglose.t+9) > peon
				&& color(J.desglose.t+7) == negro && color(J.desglose.t+9) == negro
				)
				return 1;
		}
	}
	return 0;
}

int CSort::EsSegura(CJugada J)
{
	if(EsAtacadaMaps(J.desglose.t,ColorJuegan))
	{
		int t = J.desglose.t;
		int p = board(J.desglose.f);
		if(ValorPiezas[p] > MenorDefensor(t))
			return false;
		int c = Cuenta(t);
		if(p == peon)
		{
			int dif = t - J.desglose.f;
			if(dif == 8  || dif == -8 && c < 0)
				return false;
		}
		else
		if(c <= 0)
			return false;
	}
	return true;
}
