**********************************************
*    Komodo 8 for Windows, Linux and OSX     *
**********************************************

Komodo 8 is an improvement over all previous Komodo versions and
supports multi-core computers and tablebases.

Estimated Elo gains from the last release, Komodo 7a, are about 40 points 
on one thread, 45 on two, and 50 on four or more threads, based on 
results against other top engines at 2' + 1". 

At blitz levels (2' + 1") Komodo 8 beat Komodo 7a (the last release) by 50 elo
in a direct match on one thread of 2000 games, by 57 elo on two threads 
(2500 games), by 65 elo on four threads (1000 games), and by 45 elo on one 
thread at 5' + 3". Komodo 8 defeated the top rated commercial engine by 30 
to 43 elo points in direct matches at 2' + 1" on one, two, and four threads.

Komodo 8 is expected to obtain the top rating on the majority of the 
best-known chess engine rating lists. Earlier versions of Komodo have 
won highly respected engine tournaments such as TCEC and CCT. 

Some features in Komodo 8:
 - TableBase (Syzygy) support
 - multi-core support
 - automatic SSE4.2 detection
 - evaluation improvements
   - handling of closed positions
   - king safety
   - pawn structure
 - search improvements
 - many others


===================
If you have Windows
===================

The executables for Windows are in the /Windows subdirectory.

You will find two versions:

  komodo-8-64bit.exe
  komodo-8-32bit.exe


=================
If you have Linux
=================

The binaries for Linux are in the /Linux subdirectory.

You will find two versions:

  komodo-8-linux : This version is optimized for newer architectures
  komodo-8-linux-generic : Use this version if komodo-8-linux does not work

Note that komodo-8-linux-generic computes the same moves, but is a little slower.


===============
If you have OSX
===============

The binaries for OSX are in the /OSX subdirectory.

You will find one version:

  komodo-8-64-osx


==========
HOW TO RUN
==========

Komodo is a chess engine and not a chess GUI and thus requires the use
of a third party Graphical User Interface as most modern chess engines
do.  The Komodo web site suggests a few GUI's which are available,
some of them open source and of very high quality.

Komodo, like all chess programs, has user options that will be
explained here.  These can be set or changed from the Graphical
Interface and we will focus on the non-standard ones.

table memory
------------
This parameter determines how much memory in megabytes Komodo will
allocate to various internal tables such as the pawn structure hash
table, evaluation cache and others.  If you allocate too much it can
negatively impact your performance and if you allocate too little it
can also negatively impact your performance.  The default value of 64
is probably close to optimal on most modern machines cira 2013 but you
may want to experiment with lowering (or raising) this value if you
have an entry level machine or a particularly high performance
machine.


Use Lmr
-------
LMR is a search technique that is designed to aggressively prune moves
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


Move overhead milliseconds
--------------------------
This is designed to compensate for slow user interfaces, where a
fraction of a second can be lost in the processing and transferring of
information back and forth from the engine to the interface.  This
should probably be left alone unless you see the program starting to
forfeit games due to exhausting its time.


Drawscore
---------
In some programs this is called the "contempt factor" and defines the
value of a draw.   The default is -7 which means that the program will
have a slight tendency to avoid a draw, even if its position is slightly
worse. A value of -100 would cause Komodo to avoid a draw unless it was
at least a pawn down. A value of 100 would cause Komodo to seek draws
unless it was at least a pawn ahead. In the endgame Drawscore is automatically
brought closer to zero since the chance of winning from an inferior position is
much lower in the endgame.

time usage aggressiveness
-------------------------
This provides the ability to manipulate Komodo's time control
heuristics, to make it allocation either more aggressively or less
aggressively.  The default is 0 which is what we believe is the ideal
setting but remains to be tested.  You can make it play faster by
using a negative number or you can make it allocate time more
aggressively by setting it to a positive value.  The range of values
is between -9 and +9.  It's very unlikely extreme values will be play
well, so we suggest experimenting with relatively modest changes to
the default.

Threads
-------
The default number of threads is 1 so for multi-core machines you will
want to change this value.  For ultimate performance we suggest
setting this to the number of "real" cores on your machine.

For example, on an i7 with hyper-threading, you typically have 4 "real"
cores and 4 hyper-threaded or virtual cores.  The operating system will
probably report 8 cores but we recommend setting Komodo for 4 threads
in this case. One exception may be when you use each thread to analyze a
different position (for example using the Aquarium gui in IDeA mode), in which
case you will probably get better performance by using all 8 threads in the
above example.

Hash
----
This defines the maximum amount of memory to use for the transposition
table - a major factor in the performance of modern chess programs.
The default is set to 128 which is probably more than adequate for
standard use.  However, see the document setHASH.txt for more specific
information on how this should be set for ultimate performance.


Tablebase support
-----------------
Komodo 8 supports Ronald de Man's 'Syzygy' endgame tablebases. Endgame
tablebases can be used to provide absolute knowledge about chess
positions in which only a few pieces are left on the board (at the
time of this writing, the Syzygy bases support up to 6-man positions
[including the kings]).

The Syzygy tablebases are "Distance to Zero" (DTZ) tablebases. This means
that they report, in addition to a definitive win/draw/loss (WDL) score, the
distance to the zeroing of the 50-move draw clock. Unlike other endgame
tablebases which report Distance to Mate (DTM), the Syzygy bases won't always
report the fastest win from a particular endgame position, but you can be
confident that the WDL score (and moves) provided by a tablebase probe are
accurate.

How to get the Syzygy tablebases: at the moment, the best way to get the
required files is via the file-sharing service BitTorrent (on OSX,
Transmission is a good client, on Windows and Linux, Vuze.
The necessary .torrent files can be found at
http://oics.olympuschess.com/tracker/index.php. You should get at
least the 3-4-5-man set (approx. 2 GB). The 6-man set is an additional
approx. 158 GB large and will take a while to download. The 3-4-5-man set
is also available for direct download from
http://chess.cygnitec.com/tablebases/syzygy/.

Installing the Syzygy tablebases: create a folder on your computer's hard
drive and place the fully-downloaded .rtbw (WDL files) and .rtbz (DTZ files)
in it. If you have an SSD drive, we recommend that you place the WDL files
in a folder on that drive, especially if you are using the 6-man
bases.

Understanding the Syzygy tablebase output: if the number of pieces on the board
is less than or equal to the 'Syzygy Probe Limit' [see below] or the number of
pieces supported by your installation of the Syzygy bases, whichever is less,
Komodo will report a score which can be interpreted as a win, draw or loss.
A score of +317.99 indicates a tablebase win, while a score of -317.99
indicates a loss. A score of +0.02, 0 or -0.02 indicates a draw
(+/- 0.02 is a draw which would be a win/loss if the 50-move
rule were not in effect). In a tablebase position, Komodo will immediately
return this score with a depth of 9.

Syzygy tablebase options: Komodo has a number of UCI options to
control its use of the tablebases.

Use Syzygy: when on (default), Komodo will try to use the Syzygy tablebases
specified in the 'SyzygyPath' option (see below). When off, Komodo will not
attempt to load the bases.

SyzygyPath: this defines an absolute path on your computer to the
tablebase files. On Windows, multiple paths can be separated with a
semicolon (;) character. On OSX and Windows, multiple paths should be
separated with the colon (:) character. For instance,
"C:/Tablebases/Syzygy;D:/SyzygyWDL" or "/home/me/syzygy:/media/ssd/wdl"

Syzygy Probe Depth: this determines the depth at which tablebases
will be probed during search (note that the root position will always be probed
if 'Use Syzygy' is enabled and the number of pieces on the board is less than
or equal to the 'Syzygy Probe Limit' [see below] or the number of pieces
supported by your installation of the Syzygy bases, whichever is less). A
higher number means that Komodo will wait longer to begin probing the
tablebases for any particular search. The default depth is 2 half-plies.

Syzygy Probe Limit: this determines how many pieces need to be on the board
before Komodo begins probing (even at the root). By default, this is set to 6
(the current upper limit of the Syzygy tablebases). Of course, if you have only
the 3-4-5-man set, Komodo will only begin probing the bases when a 5-man
position arises.

Syzygy 50 Move Rule: when on (default), Komodo will respect the 50-move rule
in its tablebase evaluation (a won position which requires more than 50 moves
without a pawn move or capture to win will be scored as a draw). When off,
Komodo will ignore the 50-move rule in its evaluation (the same position would
receive a winning score).

Credits
-------
Authors: Larry Kaufman, Don Dailey and Mark Lefler.

We wish to thank Jeremy Bernstein for his fine work in adding Syzygy 
tablebase support to Komodo, and our webmaster Jesse Gersenson.
