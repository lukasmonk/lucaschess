

INI file
--------

There should be a different INI file for each engine.  Sections are
composed of "variable = value" lines.  See the sample INI files in the
"example" directory.

NOTE: There can be spaces in variable names or values.  Do not use
quotes.

Globals.ini
------------
After reading the users's ini file (the default polyglot.ini or the ini file supplied on the command line),
polyglot checks if there exists a file "globals.ini" and reads this one too.
Entries in global.ini overrides the previous values,
except for "LogFile" and "NoGlobals",which are ignored.
This is usefull to set values (Hash f.i.) same for all engines.



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


Since times polyglot used '#' and ';' to indicate a comment.This conflicts
with the usage of ; as a seperator in egtb path names.
As a workaround put the full paths in qoutes
e.g. NalimovPath = "C:\Tablebases\TB4;C:\Tablebases\TB5"
Polyglot will strip the " away.
Without quotes NalimovPath = C:\Tablebases\TB4 would be send to the engine as everything
after ; is treated as comment
A couple of examples:
   Path = "C:\bases\m4" -> Path = "C:\bases\m4"
   Path = C:\bases\m4;C:\bases\m5;C:\bases\m6  -> Path = C:\Bases\m4
   Path = "C:\bases\m4;C:\bases\m5;C:\bases\m6" -> Path = C:\bases\m4;C:\bases\m5;C:\bases\m6

if outer " are needed for whatever reason:
a pair of " right next to the inner ones are a special case: 
   Path = ""C:\bases\m4;C:\bases\m5;C:\bases\m6"" -> Path = "C:\bases\m4;C:\bases\m5;C:\bases\m6"

Options
-------

These should be put in the [PolyGlot] section.

- "Affinity" (default: -1)

For windows,no effect under linux
The value is a bitvector in wich each bit represents the processors 
that a process is allowed to run on.
If the value is -1 it is ignored and the process is run like it would be normally.

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

- "ScoreWhite" (default: false)

Display scores from white's perspective

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

- "Book" (default: false)
Indicates whether a PolyGlot book should be used.  This has no
effect on the engine own book (which can be controlled with the UCI
option "OwnBook" in the [Engine] section).  In particular, it is
possible to use both a PolyGlot book and an engine book.  In that
case, the engine book will be used whenever PolyGlot is out of
book.  Remember that PolyGlot is unaware of whether the engine is
itself using a book or not.

- "Chess960" (default: false)
Play Chess960 (also called Fischer Random Chess or FRC),

- "MateScore" (default: 10000)
Mate score reported to GUI when in xboard mode.

- "BookFile" (default: book.bin)
The name of the (binary) book file.  Note that PolyGlot will look
for it in the directory it was launched from, not in the engine
directory.  Of course, full path can be used in which case the cur-
rent directory does not matter.

- "BookRandom" (default: true)
Select moves according to their weights in the book. If false the
move with the highest weight is selected.

- "BookLearn" (default: false)
Record learning information in the opening book. Naturally this
requires the opening book to be writable.

- "UseNice" (default: false)

Run the engine at nice level 5, or "NiceValue" if it set.  On some
operating systems it may be necessary to run the engine at lower
priority for it to be responsive to commands from PolyGlot while
searching.

- "NiceValue" (default: 5)

Nice levels go from -20 to 20 with 20 being the lowest priority.
On Unix only root can set negative nice levels. On Windows the
standard Win32 priority levels are mapped in a sensible way to Unix
nice levels.

- "PostDelay" (default: 0)

This will supress the pv output
from the engine for #n seconds.Usefull in comination with tlcs

- "NoGlobals" (default: false)
If true,the reading of globals.ini after the user's ini file will be skipped.
Setting this in globals.ini has no effect.

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

- "CanPonder" (default: false)
           PolyGlot now conforms to the documented UCI behaviour: the engine
           will be allowed to ponder only if it (the engine) declares the
           "Ponder" UCI option.  However some engines which can actually pon-
           der do not declare the option.  This work around lets PolyGlot know
           that they can ponder.

- "SyncStop" (default: false)
           When a ponder miss occurs, Polyglot interrupts the engine and IMME-
           DIATELY launches a new search.  While there should be no problem
           with this, some engines seem confused and corrupt their search
           board.  "SyncStop" forces PolyGlot to wait for the (now useless)
           ponder search to finish before launching the new search.

- "PromoteWorkAround" (default: false)
           Some engines do not specify a promotion piece, e.g. they send
           "e7e8" instead of the correct "e7e8q".  This work around enables
           the incorrect form (and of course promotes into a queen).

- "RepeatPV" (default: true)
           When true, PolyGlot repeats the last pv string (which also contains
           score,depth and time usage) it got from the engine. Some engines
           however do not send a new pv string just before sending the move
           and the now old pv string might confuse debugtools that parse the
           winboard debug files.


