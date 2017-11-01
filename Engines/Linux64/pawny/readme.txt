
Pawny Chess Engine - readme.txt
--------------------------------

Licence
--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------


Introduction
---------------
Pawny is UCI chess engine with estimated strength 2700 ELO CCRL / 2500 ELO CEGT.
The program supports limited console mode interface for debugging purposes.
For more information on commands supported, see below.
Occasionally, Pawny is playing at FICS using the account handle “PawnyX”.
Pawny is using common and known ideas in computer chess nowadays.
The main sources of inspiration are Crafty, Glaurung, Booot.
Many thanks to their authors for publishing their sources, always being very helpful
in discussions in the past, as well as to the people, devoting their time and enormous efforts
in computer chess to test, build rating lists and bring their results to public.


Commands:
---------------
Pawny's console mode supports the following commands:

	- uci - entering uci mode.
	- quit - exit(0);
	- go - force engine to start searching.
	- undo - undo the last move.
	- new - new game.
	- perft <depth> - performance test.
	- divide <depth> - divided perft.
	- setboard <fen> - sets the position from FEN string
	- getfen - display the FEN string, coresponding to the current pos.
	- sd <depth> - maximum search depth (time given is suppressed)
	- epdtest <filename> - starts a test, based on epd nodecounts testsuite.
	note: the moves are parsed in algebratic notation ('e2e4' 'e7e8q || e7e8Q', 'e1g1' instead of 'O-O')
	note: the default initial time per move is 50 sec.
	note: perft and divide are using another hash table with constant size of 25MB.


Technical Info
----------------
	
	Board rep. and move generation
    - Magic bitboards, reduced array access approach.
    - Legal move generator is used currently, this is likely to get changed
      when necessary optimizations take place.

	Search techniques used:
	- Iterative deepening
	- Alpha-Beta PVS + aspiration (1/5 PAWN)
	- Internal Iterative Deepening
	- Adaptive Null Move Pruning
	- Futility Pruning (including Ext. F.P. and Razoring)
	- Delta Pruning at Quiescence Search, based on SEE  results (1 PAWN value margin)
	- Late Move Reductions
	- Killer and History Heuristics
	- Transposition Table (1 table/4 entry bucket)
	- Extensions:
		• Check Extension (1 ply)

	Evaluation:
	At this time, the evaluation function is still simplistic. 
	It evaluates material, psq tables for each piece. 
	Basic terms in pawn structure are in place – doubled, hanging, passed, backward pawns.
	Piece evaluation takes into account factors that are valid game phase independently. 
	Game phase evaluation includes just a couple of things: 
		Opening evaluation with basic info for piece development, 
		Midgame evaluation – mobility, center control.  Both in combination with king safety.
		Rook and Knights material imbalance calculations. 
		Endgame evaluation – Distance and support for passed pawns.
	Static exchange evaluation - for presumably bad captures only.
  
  Endgame:
  3-4-5 Men Gaviota tablebases support. For more information, please visit: Gaviota chess engine's website .


Missing Features, Known Bugs and To-Do List
----------------------------------------------
This is a simple engine and there is a lot of room for improvement.
So far it has been tested mostly with ChessBase and BabasChess interfaces. 
It works under Arena with the remark that output is liable to further changes and improvements.
More sophisticated time control would be nice as well. 
For now is used just a simple time control management 
that only excludes new iteration if there is not enough time for it – 
also a record in the to-do list. 


Personal thanks to:
----------------
Tord Romstad, Tom Kerrigan, Dr.Wael Deeb for his support, 
Miguel Ballicora for his endgame tablebases and for always been helpful and friendly!
The helpful chess community from TalkChess and WinBoard forums!
Anyone interested in my engine.

Contact
-----------
For any impressions, remarks, deprecations, feel free to email me at:  pawnychess@gmail.com

