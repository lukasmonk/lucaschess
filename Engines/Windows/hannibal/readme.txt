Engine: Hannibal 1.7
Authors: Samuel N. Hamilton and Edsel G. Apostol
E-mail: snhamilton@rocketmail.com, ed_apostol@yahoo.com
Home Page: http://sites.google.com/site/edapostol/hannibal
Release Date: August 09, 2016
First version release: July 17 2010


This is free software. There is no warranty of any kind. Use at your own risk.


Information:

Hannibal is a collaborative work between Sam Hamilton (author of LearningLemming) and Edsel Apostol (author of TwistedLogic).  It borrows heavily from Edsel's TwistedLogic chess engine, with a more selective search and significantly improved endgame play.  No GUI is provided, Hannibal is meant to be played using a GUI such as Arena (http://www.playwitharena.com/) that supports UCI chess engines.  If there are bugs you would like to report, or features you would like added, feel free to email either of us using the contact information provided above.

UCI options:

Hash (1-65536) - hash table size in MB
Pawn Hash (1-1024) - pawn hash table size in MB, this is per Thread
Clear Hash (button) - clears the hash table
MultiPV (1-128) - search multi variations
Ponder (true/false) - whether or not engine thinks on opponent's time
OwnBook (true/false) - whether the engine uses a book
Book File(string) - name of the book file
Time Buffer(0-10000) - time in milliseconds reserved by engine for GUI related overhead
Threads (1-512) - number of threads used in SMP
Min Split Depth (1-8) - the minimum depth used for splitting the remaining moves in a YBWC SMP algorithm
Max Active Split/Threads (1-8) - the maximum number of split points per thread
Contempt(-100 - 100) - how much better or worse opponent is expected to be (negative value indicates Hannibal believes it is worse at chess than opponent, positive indicates it is better)

Todo:

Improve/Tune domain knowledge
Persistent Hash for analysis
Chess 960
Book/Positional Learning
GUI w/Personalities, Tutorial Mode, Opening Analysis, Game Analysis
Syzygy Tablebases

Acknowledgement:

Big thanks to these people, this engine wouldn't be this strong without them:
Audy Arandela (USA) - for the valuable tests, for the very strong opening book used in tournaments, for being an operator for online tournaments, for running the engine in online tournaments and in ICC
Timo Haupt (Germany) - for being a tester and operator for Hannibal in the 47th edition of the CSVN programmer's tournament

Thanks to Pradu Kannan for the magic moves generation for sliding pieces.
Thanks to Tord/Marco/Joona and the contributors to the Stockfish project where we borrowed a lot of ideas and Tord again for SMP ideas in Viper.
Thanks to the author of Fruit, Fabien Letouzey, for providing the source code of his program, as we both learned a lot from it.
Thanks to the author of Gull, ThinkingALot, for the ideas on magic bitboard move generation initialization stuff for sliding pieces used in Gull.
Thanks to the author of Buzz OS, Pradu Kannan, for the ideas on pawn structure evaluation. 
Thanks to Michel Van den Bergh for the polyglot book specification and code.
Thanks to Michael Sherwin for sharing his ideas, wit, and source code.
Thanks to all the people on CCC, Winboard, WBEC, Open-Chess forums, etc. We have learned a lot from their posts. 
Thanks to all the programmers who shared their source code (anonymously or otherwise) without which Hannibal would clearly be a weaker program. 
Thanks also to all the people who runs various tournament and rating lists that includes our program (TCEC, CEGT, CCRL, IPON, FCP, etc.)
Thanks to Graham Banks, Jean-Paul Vael and Clemens Keck for playing the initial and later Hannibal SMP beta in their tournaments.
Thanks to Gerd Isenberg for maintaining the chess programming wiki which is an important knowledge resource.
And lastly, thanks to those that we forgot to mention.

Changes:

1.0
-first release

1.0a
-fixed a bug in the opening book implementation

1.1
-rewrote most of the search
-eval parameter tuning

1.2
-search improvements
-eval improvements

1.3
-search improvements
-SMP

1.4
-search improvements
-improved SMP
-minor eval improvements

1.4a
-fixed Polyglot book support

1.4b
-fixed illegal moves bug from Polyglot book

1.5 (January 28, 2015)
-rewrote most of the support framework
-improved hash table handling
-improved SMP, now using C++11 threads
-improved endgame knowledge
-improved search including move selection, prunings, reductions, etc.
-Multi-PV support
-pawn hash and eval cache are per threads now

1.7 (August 09, 2016)
-Rewrote Hash tables (condensed move representation, combined main hash and eval hash).
-Rewrote SMP (better work allocation, more efficient splits&copies, increased thread support).
-Rewrote Search/Qsearch (refactored, fail high prediction & research, increased graduating reductions, various additional pruning & reductions, improved time management).
-Rewrote Eval (retuned material values, merged eval scoring, improved outpost evaluation, improved knight position evaluation, new mobility, improved pawn structure evaluation, lock position detection in endgame, various other endgame improvements).

