************************************
*  Komodo 5 for Windows and Linux  *
************************************

Komodo 5 has many improvements and bug fixes over Komodo 4 and will play 
a much stronger game.   

 - automatic SSE4.2 detection
 - several evaluation improvements
 - time control improvements
 - search is better
 - multipv implemention improved
 - Hash table utilization better
 - many others


===================
If you have Windows
===================

The executables for Windows are in the /Windows subdirectory.

You will find 1 version:

  komodo-5-64bit.exe


=================
If you have Linux
=================

The binaries for Linux are in the /Linux subdirectory.

You will find 1 version:

  komodo-5-64bit



==========
HOW TO RUN
==========

Komodo is a chess engine and not a chess GUI and thus requires the use
of a third party Graphical User Interface as most modern chess engines
do.  The Komodo web site suggests a few GUI's which are available,
some of them open source and of very high quality.

Komodo, like all chess programs, has some user options that will be
explained here.  These can be set or changed from the Graphical
Interface and we will focus on the non-standard ones.


Use Lmr
-------
LMR is a seach technique that is designed to aggressively prune moves
from the search tree so much deeper depths can be obtained. However
this can sometimes miss important moves.  Komodo gives you the option
to turn this on or off.  In general Komodo will play much stronger
with this option turned on, but one might want to experiment with
turning it off in order to see if a much shallower, but more thorough
search might help in some position.


Null Move pruning
-----------------
Like LMR, this is also a technique to aggressively prune moves from
the search tree.  Komodo plays much stronger with this turned ON, but
occasionally things can be missed, especially very deep threats or
zugzwang positions.  This option allows you to experiment in the case
that you have a difficult position which cannot be resolved and want to
see if a much shallower, but more thorough, search would help.


Deterministic 
-------------
If this is set, the program plays a bit weaker in general, but the
search is guaranteed to be deterministic or repeatable.  Komodo is
not guaranteed to play the same move, with the same score and node
counts, in a given position. But, if this option is set it will.


Strength
--------
One problem with Komodo is that it is almost impossible to beat.  So
Komodo allows you to set its strength to a value between 0-15.  A
value of zero is the weakest play and a value of 15 is the default,
and the strongest.

The strength function basically slows the program down very
signficantly so that it cannot search as deeply.  At level zero the
program will take a lot of time just to complete 2 or 3 levels (or
ply) of searching.  It should be used in conjunction with normal time
control play where the computer gets the same amount of time as you
do.  The more time you give the machine the stronger it will play, but
that is true of human players too.  So as a training tool you can play
Komodo at level zero and if you beat it 50% of the time move it up to
the next level and so on.  But be careful!  Even at level zero you
mustn't hang a piece, Komodo will see it and punish you!


Move overhead milliseconds
--------------------------
This is designed to compensate for slow user interfaces, where a
fraction of a second can be lost in the processing and transfering of
information back and forth from the engine to the interface.  This
should probably be left alone unless you see the program starting to
fofeit games due to exhausting its time.


Drawscore
---------
In some programs this is called the "contempt factor" and defines the
value of a draw.   The default is -5 which means that the program will
have a slight tendency to avoid a draw, even if its position is slightly
worse. A value of -100 would cause Komodo to avoid a draw unless it was
at least a pawn down. A value of 100 would cause Komodo to seek draws
unless it was at least a pawn ahead.  

