//    Copyright 2009-2012 Antonio Torrecillas Gonzalez
//
//    This file is part of Rocinante.
//
//    Rocinante is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Rocinante is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Rocinante.  If not, see <http://www.gnu.org/licenses/>
//

#include <string.h>
#include <fstream>
#include <iostream>
using namespace std;
#include <assert.h>

#include "ufo.h"

/*
The content of this page has been originally posted by Tomasz Michniewski 
on the Polish chess programming discussion list (progszach). 
It contains a proposal of extremely basic evaluation function required 
from the participants of a "Unified Evaluation" test tournament. 
Please note that the values presented here have been designed specifically to 
compensate for the lack of any other chess knowledge, 
and not for being supplemented by it.

There are two parts of this email, the first is about simple piece values, 
the second is about piece/square tables.


Piece Values
Setting piece values I wanted to achieve several results:

Avoid exchanging one minor piece for three pawns. 
Encourage the engine to have the bishop pair. 
Avoid exchanging of two minor pieces for a rook and a pawn. 
Stick to human chess experience. 

Point 1 is achieved by simple:

B > 3P
N > 3P
Of course there are positions where three (or even two) pawns are better than a piece. 
But generally, exchanging of a minor piece for three pawns is a bad idea, 
as these pawns might come under attack and after capturing one of them the position 
becomes critical. So in longer term it is better to have a piece, not three pawns.

Point 2 is achieved by:

B > N
Of course this doesn't guarantee anything, because we may end up with just one bishop 
against one knight. But on the other hand chess players often capture bishop with knight 
while rarely capture knight with bishop. For example white bishop d3 rarely captures 
black knight standing on central square e4. On the contrary, white knight on e4 may 
easily capture black bishop at f6. Of course one may say that sometimes white bishop 
from g5 captures black knight at f6. But this often happens after forcing h6 or just 
time issues are involved, which we ignore here. The same with Nimzowitsch bishop at b4, 
which should capture knight at c3 only after being forced with a3 let say.

So, we avoid unnecessary exchanging of a bishop for a knight. We increase the chance 
to have the bishop pair. On the other hand we risk to end up in an ending with either 
a wrong bishop or just a bishop over a knight with pawns on only one side or in closed 
position. So additional evaluation with looking at pawns' constellation is called for, 
but at this stage we can't afford this.

So together we may say:

B > N > 3P
Point 3 is achieved by:

B + N > R + P
This is a bit problematic, as once I read in Russian Shachmaty that two minor pieces 
are worth a rook and two pawns. However, this might be an exageration. I may recall 
game from Karpov-Kasparov match where Kasparov could have won the ending with R + 2P 
against two minor pieces (plus some extra pawns on both sides). The game ended up 
in a draw, but a rook plus two pawns might be a bit too much. So instead of:

R + 2P > B + N > R + P
let's say something between:

B + N = R + 1.5P
OK, now I'd like to call for the word "symmetry". In chess the more symmetric position 
and material balance, the bigger probability of a draw result. Here, since the material 
balance isn't symmetric, we underline this with 1.5 factor, so the engine will either 
tend to have two minors against a rook plus a pawn or a rook plus two pawns against 
two minors. Please remember that this is still material evaluation, we aren't even 
in piece/square tables, not talking about real positional evaluation - so there is huge room 
for improvement in the future.

Additional comment to the last equation is that two knights would be a bit weaker than 
R + 1.5P, while two bishops would be a bit stronger, but this is not important now.

As you see I try to stick to the rule 4 as well. The final equation I would refer to is:

Q + P = 2R
Together we have:

B > N > 3P
B + N = R + 1.5P
Q + P = 2R
which might be satisfied by (values in centi-pawns):

P = 100
N = 320
B = 330
R = 500
Q = 900
K = 20000
Additionally we can see that they satisfy another relationship:

B + 2P > N + 2P > R

However I didn't like to use it, because to exchange a rook for a minor piece and two pawns 
it is not enough to simply capture a knight with a rook and then additionally capture the 
recapturing pawn, because we additionally need two extra plies for capturing of another pawn.
So this isn't often - instead just a rook for a minor piece plus one pawn is more often, 
where additionally some compensation is achieved. But this isn't our subject at the moment.

According to these settings:

2B + N > Q
and
R + B + P > Q (or R + N + P > Q as well)
I don't know what is Adam's or Mateusz's opinion about these relations and all the settings. 
I don't like last two formulas, but on the other hand I would like to stick to the classical 
rule:

Q + P = 2R
Additional comment to:

K = 20000
This is Shannon's value for the king, which sometimes might be useful for discovering whether 
the king was taken. It has this feature that the maximal material balance has a 
value of 20000 + 9*900 + 2*500 + 4*300 = 30300 (approx.) 
and might be stored as 2 byte integer with a sign, so if someone wants to store two such values 
in one 4 byte integer - why not.

Another comment is that sometimes material balance might be perceived as investing on the stock 
exchange - we should look at the results in longer perspective. I'd like to recall the 
example with a game, where one side wins without moving Ra1, which waited silently for 
development of Nb1, which wasn't touched at all. This side somehow won. Now let's ask 
whether White would win if there was no white rook at a1 from the beginning. 
Probably not. Why? Because there would appear exchange variants which would favour Black. 
So the material balance played its role. But the results of the presence of white rook at a1 
could be seen only after several moves, maybe in the ending.


Piece/Square Tables
Now, let's move for piece/square values. Generally I would like to give bonuses for pieces 
standing well and penalties for pieces standing badly. Other squares will have neutral 
value of 0. Of course one may incorporate above piece values into piece/square tables 
if they wish. I don't do this.


Pawns
For pawns we simply encourage the pawns to advance. Additionally we try to discourage 
the engine from leaving central pawns unmoved. The problem I could see here is that this 
is contradictory to keeping pawns in front of the king. We also ignore the factor whether 
the pawn is passed or not. So more advanced evaluation is called for, especially 
that "pawns are soul of the game".

// pawn
 0,  0,  0,  0,  0,  0,  0,  0,
50, 50, 50, 50, 50, 50, 50, 50,
10, 10, 20, 30, 30, 20, 10, 10,
 5,  5, 10, 25, 25, 10,  5,  5,
 0,  0,  0, 20, 20,  0,  0,  0,
 5, -5,-10,  0,  0,-10, -5,  5,
 5, 10, 10,-20,-20, 10, 10,  5,
 0,  0,  0,  0,  0,  0,  0,  0
 
 OK, let's comment this table. Firstly the shelter in front of white short castle 
 (long castle - it's symmetrical) - pawns at f2, g2 and h2 get bonuses. Additionally 
 we set negative values for f3 and smaller for g3 which both create holes around king. 
 Pawn h2 have the same value on h2 and h3, so the engine may create the hole if the 
 situation calls for it. Moreover - if it gets position with a pawn at g3 and a 
 bishop at g2, then it still may play h3 or not. Therefore h3 has the same value as h2.

Zero value on f4, g4, h4 prevents playing with pawns in front of the king. Moving these 
pawns to f5, g5, h5 still brings nothing, but at this moment we have the same values as 
on rank 2.

In the centre we have the most negative values on d2 and e2. We don't like these pawns. 
d3 and e3 aren't good either. Only d4 and e4 in the centre. Even better on d5 and e5.

Beginning with rank 6th we give bonus for advanced pawns. On rank 7th even bigger.


Knights
With knights we simply encourage them to go to the center. Standing on the edge is a bad idea.
Standing in the corner is a terrible idea. Probably it was Tartakover who said that
"one piece stands badly, the whole game stands badly". And knights move slowly.

// knight
-50,-40,-30,-30,-30,-30,-40,-50,
-40,-20,  0,  0,  0,  0,-20,-40,
-30,  0, 10, 15, 15, 10,  0,-30,
-30,  5, 15, 20, 20, 15,  5,-30,
-30,  0, 15, 20, 20, 15,  0,-30,
-30,  5, 10, 15, 15, 10,  5,-30,
-40,-20,  0,  5,  5,  0,-20,-40,
-50,-40,-30,-30,-30,-30,-40,-50,

As you can see I would happily trade for three pawns any knight standing on the edge.
Additionally I put slight bonuses for e2, d2, b5, g5, b3 and g3. Then there are bonuses 
for being in the center.


Bishops
// bishop
-20,-10,-10,-10,-10,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5, 10, 10,  5,  0,-10,
-10,  5,  5, 10, 10,  5,  5,-10,
-10,  0, 10, 10, 10, 10,  0,-10,
-10, 10, 10, 10, 10, 10, 10,-10,
-10,  5,  0,  0,  0,  0,  5,-10,
-20,-10,-10,-10,-10,-10,-10,-20,
We avoid corners and borders. Additionally we prefer squares like b3, c4, b5, d3 
and the central ones. Moreover, I wouldn't like to exchange white bishop at d3 (or c3) 
for black knight at e4, so squares at c3 (f3) have value of 10. As a result white bishop 
at d3 (c3) is worth (330+10) and black knight at e4 is worth (320+20). So the choice of 
whether to exchange or not should depend on other issues. On the contrary white bishop 
at e4 (330+10) would be captured by black knight from f6 (320+10). White bishop at g5 
(330+5) won't capture black knight at f6 (320+10).


Rooks
rook
  0,  0,  0,  0,  0,  0,  0,  0,
  5, 10, 10, 10, 10, 10, 10,  5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
  0,  3,  4,  5,  5,  0,  0,  0
  The only ideas which came to my mind was to centralize, occupy the 7th rank and avoid a, h 
  columns (in order not to defend pawn b3 from a3). So generally this is Gerbil like.


Queen
//queen
-20,-10,-10, -5, -5,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5,  5,  5,  5,  0,-10,
 -5,  0,  5,  5,  5,  5,  0, -5,
  0,  0,  5,  5,  5,  5,  0, -5,
-10,  5,  5,  5,  5,  5,  0,-10,
-10,  0,  5,  0,  0,  0,  0,-10,
-20,-10,-10, -5, -5,-10,-10,-20
Generally with queen I marked places where I wouldn't like to have a queen. Additionally 
I slightly marked central squares to keep the queen in the centre and b3, c2 squares 
(Pawel's suggestion). The rest should be done by tactics.


King
king middle game
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-20,-30,-30,-40,-40,-30,-30,-20,
-10,-20,-20,-20,-20,-20,-20,-10,
 20, 20,  0,  0,  0,  0, 20, 20,
 20, 30, 10,  0,  0, 10, 30, 20
 This is to make the king stand behind the pawn shelter.

In the ending the values change.

// king end game
-50,-40,-30,-20,-20,-30,-40,-50,
-30,-20,-10,  0,  0,-10,-20,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 30, 40, 40, 30,-10,-30,
-30,-10, 20, 30, 30, 20,-10,-30,
-30,-30,  0,  0,  0,  0,-30,-30,
-50,-30,-30,-30,-30,-30,-30,-50

And of course "These values are for white, for black I use mirrored values.ç
" Additionally we should define where the ending begins. For me it might be either if:

Both sides have no queens or 
Every side which has a queen has additionally no other pieces or one minorpiece maximum. 


*/

static const int Defectos[7][64] = 
{
	{ // 0 peon
		 0,  0,  0,  0,  0,  0,  0,  0,
		 5, 10, 10,-20,-20, 10, 10,  5,
		 5, -5,-10,  0,  0,-10, -5,  5,
		 0,  0,  0, 20, 20,  0,  0,  0,
		 5,  5, 10, 25, 25, 10,  5,  5,
		10, 10, 20, 30, 30, 20, 10, 10,
		50, 50, 50, 50, 50, 50, 50, 50,
		 0,  0,  0,  0,  0,  0,  0,  0
	},
	{ // 1 caballo
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50,
	},
	{ // 2 alfil
		// bishop
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  5,  0,  0,  0,  0,  5,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10,-10,-10,-10,-10,-20,
	},
	{ // 3 torre
		  0,  3,  4,  5,  5,  0,  0,  0
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		 -5,  0,  0,  0,  0,  0,  0, -5,
		  5, 10, 10, 10, 10, 10, 10,  5,
		  0,  0,  0,  0,  0,  0,  0,  0,
	},
	{ // 4 dama
		//queen
		-20,-10,-10, -5, -5,-10,-10,-20
		-10,  0,  5,  0,  0,  0,  0,-10,
		-10,  5,  5,  5,  5,  5,  0,-10,
		  0,  0,  5,  5,  5,  5,  0, -5,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		-10,  0,  5,  5,  5,  5,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20,
	},
	{ // 5 rey
		//king middle game
		 20, 30, 10,  0,  0, 10, 30, 20,
		 20, 20,  0,  0,  0,  0, 20, 20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
	},
	{ // 6 rey final
		// king end game
		-50,-30,-30,-30,-30,-30,-30,-50
		-30,-30,  0,  0,  0,  0,-30,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-50,-40,-30,-20,-20,-30,-40,-50,
	},
};

ufo PstInitialize;

ufo::ufo(void)
{
	Initialize();
}

ufo::~ufo(void)
{
}

ufo ufoEvaluation;

extern int Pst[12][64][2]; // piece,square,stage

void ufo::Initialize()
{
//	const int ValuePiece[7] = {100,320,330,500,900,0,0};
	int sq,chessman;

	for(chessman = 0; chessman < 5;chessman++)
	{
		for(sq = 0;sq < 64;sq++)
		{
			Pst[chessman*2][sq][0] = Pst[chessman*2][sq][1] = Defectos[chessman][sq];
		}
	}
	for(chessman = 0; chessman < 5;chessman++)
	{
		for(sq = 0;sq < 64;sq++)
		{
			Pst[1+chessman*2][sq^070][0] = Pst[1+chessman*2][sq^070][1] = -Defectos[chessman][sq];
		}
	}
	for(sq = 0;sq < 64;sq++)
	{
		Pst[10][sq][0] = Defectos[5][sq];
	}
	for(sq = 0;sq < 64;sq++)
	{
		Pst[10][sq][1] = Defectos[6][sq];
	}
	for(sq = 0;sq < 64;sq++)
	{
		Pst[11][sq^070][0] = -Defectos[5][sq];
	}
	for(sq = 0;sq < 64;sq++)
	{
		Pst[11][sq^070][1] = -Defectos[6][sq];
	}

}
