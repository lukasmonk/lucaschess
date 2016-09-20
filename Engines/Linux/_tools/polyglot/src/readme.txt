
Legal details
-------------

PolyGlot 1.3 Copyright 2004-2005 Fabien Letouzey.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

See the file "copying.txt" for details.


General
-------

PolyGlot 1.3 (2005/06/03).

PolyGlot is a "UCI adapter".  It connects a UCI chess engine to an
xboard interface such as WinBoard.  UCI2WB is another such adapter
(for Windows).

PolyGlot tries to solve known problems with other adapters.  For
instance, it detects and reports draws by fifty-move rule, repetition,
etc ...


Official distribution URL
-------------------------

The official distribution web site is Leo Dijksman's WBEC Ridderkerk:
http://wbec-ridderkerk.nl/  This is where you should be looking for
PolyGlot updates in the future.


Install
-------

PolyGlot should have its own directory.  The INI files for engines
should also be put there.  Dirtier solutions are needed when
BookThinker is used though.

On Windows the files "polyglot.exe" and "cygwin1.dll" (which you can
download from http://wbec-ridderkerk.nl/) are needed.  On Linux and
Mac OS X only the file "polyglot_linux" or "polyglot_mac" is required.


Compiling
---------

The distribution comes up with Windows, Linux and Mac OS X binaries.
Compiling is therefore not necessary on those systems unless you want
to make a change in the program.  In any case this section describes
the compiling procedure, it is safe to skip it.

PolyGlot is a POSIX application (Unix compatible), and was developed
on Linux using g++ (the GNU C++ compiler).

1) Unix

You should be able to compile it on any POSIX-compliant operating
system (*not* Windows) with the following command line (or similar):

> g++ -O2 -o polyglot *.cpp

IMPORTANT: In "io.cpp", the variable "UseCR" should be set to "false".

A Makefile is provided but might not work on your system ...

2) Windows

On Windows, you *must* use Cygnus GCC to compile PolyGlot.

IMPORTANT: In "io.cpp", the variable "UseCR" should be set to "true".


Usage
-----

PolyGlot acts as an xboard engine.  There should be no difference with
a normal chess program as far as the interface (e.g. WinBoard) is
concerned.

PolyGlot is invoked using "polyglot <INI file>".  Note that PolyGlot
will look for the INI file in the current directory.  If no <INI file>
is given, "polyglot.ini" is selected.

To use PolyGlot with XBoard, you would type something like this:
> xboard -fd 'polyglot_dir' -fcp 'polyglot engine.ini'

Quotes are important when there is a space in the argument.


INI file
--------

There should be a different INI file for each engine.  Sections are
composed of "variable = value" lines.  See the sample INI files in the
"example" directory.

NOTE: There can be spaces in variable names or values.  Do not use
quotes.

1) [PolyGlot] section

This section is used by PolyGlot only.  The engine is unaware of these
options.  The list of available options is detailed below in this
document.

2) [Engine] section

This section contains engine UCI options.  PolyGlot does not
understand them, but sends the information to the engine at startup
(converted to UCI form).  You can add any UCI option that makes sense
to the engine (not just the common options about hash-table size and
tablebases).

NOTE: use INI syntax, not UCI.  For example "OwnBook = true" is
correct.  It will be replaced by PolyGlot with "setoption name OwnBook
value true" at engine startup.

Standard UCI options are "Hash", "NalimovPath", "NalimovCache" and
"OwnBook".  Hidden options like "Ponder" or "UCI_xxx" are automatic
and should not be put in an INI file.

The other options are engine-specific.  Check their name using a UCI
GUI or launch the engine in a console and type "uci".


Options
-------

These should be put in the [PolyGlot] section.

- "EngineName" (default: UCI name)

This is the name that will appear in the xboard interface.  It is
cosmetic only.  You can use different names for tweaked versions of
the same engine.

If no "Engine Name" is given, the UCI name will be used.

- "EngineDir" (default: ".")

Full path of the directory where the engine is installed.  You can use
"." (without the quotes) if you know that PolyGlot will be launched in
the engine directory or the engine is in the "path" and does not need
any data file.

- "EngineCommand"

Put here the name of the engine executable file.  You can also add
command-line arguments.  Path searching is used and the current
directory will be "EngineDir".

NOTE: Unix users are recommended to prepend "./"; this is required on
some secure systems.

- "Log" (default: false)

Whether PolyGlot should log all transactions with the interface and
the engine.  This should be necessary only to locate problems.

- "LogFile"

The name of the log file.  Note that it is put where PolyGlot was
launched from, not into the engine directory.

WARNING: Log files are not cleared between sessions, and can become
very large.  It is safe to remove them though.

- "Resign" (default: false)

Set this to "true" if you want PolyGlot to resign on behalf of the
engine.

NOTE: Some engines display buggy scores from time to time although the
best move is correct.  Use this option only if you know what you are
doing (e.g. you always check the final position of games).

- "ResignMoves" (default: 3)

Number of consecutive moves with "resign" score (see below) before
PolyGlot resigns for the engine.  Positions with only one legal move
are ignored.

- "ResignScore" (default: 600)

This is the score in centipawns that will trigger resign "counting".

- "ShowPonder" (*** NEW ***, default: true)

Show search information during engine pondering.  Turning this off
might be better for interactive use in some interfaces.

- "KibitzMove" (*** NEW ***, default: false)

Whether to kibitz when playing a move.

- "KibitzPV" (*** NEW ***, default: false)

Whether to kibitz when the PV is changed (new iteration or new best move).

- "KibitzCommand" (*** NEW ***, default: "tellall")

xboard command to use for kibitzing, normally "tellall" for kibitzing
or "tellothers" for whispering.

- "KibitzDelay" (*** NEW ***, default: 5)

How many seconds to wait before starting kibitzing.  This has an
affect only if "KibitzPV" is selected, move kibitzes are always sent
regardless of the delay.


Work arounds
------------

Work arounds are identical to options except that they should be used
only when necessary.  Their purpose is to try to hide problems with
various software (not just engines).  The default value is always
correct for bug-free software.

IMPORTANT: Any of these work arounds might be removed in future
versions of PolyGlot.  You are strongly recommended to contact the
author of faulty software and truly fix the problem.

In PolyGlot 1.3 there is only one optional work around:

- "UCIVersion" (default: 2)

The default value of 2 corresponds to UCI+.  Use 1 to select plain
UCI for engines that have problems with UCI+.


Opening Book
------------

*** NEW ***

PolyGlot 1.3 provides a minimal opening-book implementation.

New options can be added to the [PolyGlot] section:

- "Book" (default: false)

Indicates whether a PolyGlot book should be used.  This has no effect
on the engine own book (which can be controlled with the UCI option
"OwnBook" in the [Engine] section).  In particular, it is possible to
use both a PolyGlot book and an engine book.  In that case, the engine
book will be used whenever PolyGlot is out of book.  Remember that
PolyGlot is unaware of whether the engine is itself using a book or
not.

- "BookFile"

The name of the (binary) book file.  Note that PolyGlot will look for
it in the directory it was launched from, not in the engine directory.
Of course, full path can be used in which case the current directory
does not matter.

Note that there is no option to control book usage.  All parameters
are fixed when compiling a PGN file into a binary book (see below).
This is purposeful and is not likely to change.

Using a book does not require any additional memory, this can be
important for memory-limited tournaments.

A default book "fruit.bin" is provided in the archive.  Note that this
book is very small and should probably not be used in serious games.
I hope that users will make other books available in the future.


Book Making
-----------

*** NEW ***

You can compile a PGN file into a binary book using PolyGlot on the
command line.  At the moment, only a main (random) book is provided.
It is not yet possible to control opening lines manually.  I am
working on it though.

Usage: "polyglot make-book <options>".

"make-book" options are:

- "-pgn"

Name of the input PGN file.  PolyGlot should support any
standard-conforming file.  Let me know if you encounter a problem.

- "-bin"

Name of the output binary file.  I suggest ".bin" as the extension but
in fact PolyGlot does not care.

- "-max-ply" (default: infinite)

How many plies (half moves) to read for each game.  E.g. if set to
"20", only the first 10 full moves of each game will be scanned.

- "-min-game" (default: 3)

How many times must a move be played to be kept in the book.  In other
words, moves that were played too rarely will be left out.  If you
scan full games "2" seems a minimum, but if you selected lines
manually "1" will make sense.

Example: "polyglot make-book -pgn games.pgn -bin book.bin -max-ply 30".

Building a book is usually very fast (a few minutes at most).  Note
however that a lot of memory may be required.  To reduce memory usage,
select a ply limit.


History
-------

2004/04/30: PolyGlot 1.0

- first public release.

2004/10/01: PolyGlot 1.1

- added "StartupWait" and "PonderWorkAround" ("AutoQuit" was available
  in version 1.0 but not documented).

- fixed a minor bug that could prevent "AutoQuit" from working with
  some engines.

2005/01/29: PolyGlot 1.2

- rewrote engine initialisation and UCI parsing to increase
  UCI-standard compliance

- added multi-move resign

- added an internal work around for engines hanging with WinBoard

2005/06/03: PolyGlot 1.3

- added opening book

- added kibitzing

- added "ShowPonder" option



Known bugs
----------

*** IMPORTANT ***

There is a bug (!) in the xboard automaton.  The bug is related to
searching in draw positions (e.g. 50-move rule or repetition).  I had
only been able to make PolyGlot crash by using analysis mode and
performing manual takebacks.  I believe that this bug can only happen
in highly-interactive use (e.g. manual analysis).  It is present in
all versions of PolyGlot, including this one.

I attempted a work around in February.  I vaguely remember the change
prevents crashing (not sure) but it is possible that PolyGlot now gets
stuck in some rare case, i.e. it refuses to produce a move.  In any
case, the bug cannot occur silently, e.g. in a game that terminated
normally.

Because of the small expected impact (nobody ever reported it to me)
and because fixing the bug would require a whole redesign of the
xboard module, I have no intention of working on it at the moment (!).

Make sure to let me know if it appeared in any circounstance, thanks!
In particular if the bug ever occurs during a non-interactive session
(e.g. engine vs. engine game), then I will do something.


Thanks
------

Big thanks go to:

- Dann Corbit for spending a lot of time compiling, testing, making
  files available, etc ...

- Leo Dijksman for hosting the PolyGlot distribution on his web site
  (see Links) and also for thorough testing

- Tord Romstad, Joshua Shriver and George Sobala for compiling and
  testing on Mac OS X

- users in the WinBoard forum for their feedback and encouraging
  words: Roger Brown, Leo Dijksman, Igor Gorelikov, Mogens Larsen,
  Volker Pittlik, Norm Pollock, Günther Simon and Salvo Spitaleri
  in particular


Links
-----

- Tim Mann's Chess Pages: http://www.tim-mann.org/xboard.html
- Leo Dijksman's WBEC Ridderkerk: http://wbec-ridderkerk.nl/
- Volker Pittlik's Winboard Forum: http://wbforum.volker-pittlik.name/


Contact me
----------

You can contact me at fabien_letouzey@hotmail.com

If I am not available, you can discuss PolyGlot issues in Volker
Pittlik's Winboard Forum: http://wbforum.volker-pittlik.name/

In fact for questions regarding specific Windows-only engines, you are
advised to ask directly in the WinBoard forum, as I don't have Windows
myself.


The end
-------

Fabien Letouzey, 2005/06/03.

