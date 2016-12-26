=== PolyGlot 1.4W ====
Polyglot 1.4W is a modified polyglot 1.4 from Fabien Letouzy.
Just like polyglot 1.3w(see below) it can be compiled and run under windows without
the need for a cygwin1.dll

Eric Mullins made some changes so it will print pv lines correctly when polyglot is compiled using older
microsoft compilers (especially vs2003,vs2002 and vc++ 6.0)
also Eric implemented EngineNice and ScoreWhite
Best
Fonzy
www.geenvis.net
===============1.4w27=================
As winboard sends the info strings to fics/ics as well and this can cause unwanted side-effects,polyglot does not send them to winboard unless InfoStrings is set true.
new MultiPVall bool.If set true will also show the multipv lines from previous depths.

===============1.4w26=================
send info strings to winboard.
send tablebase hits and hashtable usage.
better handling of book_open error,engine path error,engine open error
multipv output changed with hgm's patch
don't resign on illegal moves while in ics mode.(no idea what will happen though ...)
===============1.4w25=================

fixed: Illegal moves resulted in polyglot waiting for xboard to declare a timeloss
	Polyglot now forfeits with the comment {Polyglot: Illegal engine move [White|Black] } 
If Resign is used and the number of ResignMoves is reached,
	Polyglot just resigns,instead  of resigning right after sending the last engine move
untested: ics: a drawing move from the uci engine will be preceded with "offer draw"
===============1.4w24=================
fixed another racing condition.
fix compiler complaint UCHAR_MAX*
makefile.gcc EXE renamed from polyglot.exe to polyglot*
included generic makefile*
*Heinz van Saanen
===============1.4w23=================
fix: Create books from pgn files that have FEN tags in the games,this should work now. 
===============1.4w22=================
Allow ; in strings such as EGTB path names as a seperator.
===============1.4w21=================
On Windows: Not much variety in book lines: forgot to seed rand() in the thread as well.
Also replaced the seeding with a better one.
===============1.4w20=================
bookmoves with zero weight are allowed,these moves always are ignored.

===============1.4w19=================
Jim Ablett fixes for non-windows compilations

===============1.4w18=================
resign in ponder games was broken,fixed

===============1.4w17=================
small fix,pg was sending game results before the last move was send to xboard.

===============1.4w16=================
Found and fixed a couple of race conditions while runing polyglot on a single core computer.


===============1.4w15=================
Removed polling.
The windows version now uses a seperate thread and normal I/O (blocking) calls.
Lag reduction is substantial!
Running epd tests is (probably) broken now and won't be fixed.

bugger! fixed LogFile = xxxxx
fixed   typo  Adapter_>Engine => Adapter->Engine

===============1.4w14===================
Skipped  versionnumber for the supersticious :P

After reading the users's ini file (the default polyglot.ini or the ini file supplied on the command line),
polyglot checks if there exists a file "globals.ini" and reads this one too.
Entries in global.ini overrides the previous values,
except for "LogFile" and "NoGlobals",which are ignored.
This is usefull if you want to set some values (Hash f.i.) same for all engines.

added a new option for the [polyglot] section in the user's ini file:
- NoGlobals : if set true, the globals.ini will not be read.

================1.4w12===================
setting priority and affinity is now done in engine_open().
log ployglot version.

various sourcecode cleanups
	use engine_close() instead of pipeengine.close();
	moved all pipestruct stuff to engine.cpp and pipe.h
	usenice and nicevalue as in michel polyglot version
	log code in engine.cpp and analyse.cpp cleaned up

++++++++++++++++++
=========1.4w11=================================
changes to adapter.ccp for older msc compilers.
added PostDelay parameter.This will supress the pv output
from the engine for #n seconds.Usefull in comination with tlcs

=========1.4w10-EM2(internal version)=============
added BOOL ScoreWhite option (always display scores from white's perspective)
added int EngineNice option
  for linux: as per nice()
  for windows:
    -19..-16: HIGHEST
     -1..-15: HIGHER
           0: NORMAL
       1..14: LOWER
      15..19: IDLE
    ** NEGATIVE VALUES WILL SCREW THINGS UP **
    This is no magic bullet for more CPU power.  The purpose is to REDUCE engine priority so
    that other processes on your computer will not be sluggish.

=========1.4w10================
fixed analysis output for toga
=========1.4w9=================
fixed bug in 1.4w8
added RepeatPV workaround
	fixed disappearing output in engine-output window while in multipv mode
	when an engine sends its move to polyglot,
	polyglot normally repeats the last pv string(which also contains score,depth and time usage) it got from the engine.
	Some engines however do not send a new pv string just before sending the move and the now old pv string
	find might confuse debugtools that parse the winboard debug files.
	Adding "RepeatPV = false" to the [POLYGLOT] section of the ini file stops this repetition.
 
=========1.4w8=================
fixed multipv output
note that the pv with the worst score will be on top of the engine-output window.
added timestamp in logfile (Jaap Weidemann)

=========1.4w7=================
compiles under linux/unix again
=========1.4w6=================
access to winboard draw/drawoffer and resign,at engine level:

	1:to activate draw offers the engine has 
	   to define the "UCI_DrawOffers" parameter with the 'option" command at startup. 
	2:to offer a draw or accept a draw offer:just send "info string DrawOffer" to polyglot. 
	3:if winboard sends "draw" polyglot sends "setoption DrawOffer draw" to the engine.
	4.to resign: send "info string Resign" to polyglot.
	please check the winboard documentation for the draw/drawoffer and resign commands.
		
=========1.4w5:================
Fixed errors in SyncStop handling.
book building: the error message now also contains the game number 
added Affinity option:
In the [PolyGlot] section(ini file):
- "Affinity" mask
mask is a bit vector in which each bit represents the processors that a process is allowed to run on.

some minor bugs fixed

checks if child did really quit.

*********************************************************
=== PolyGlot 1.3W ReadMe ===

PolyGlot - a UCI-to-WinBoard Adapter for UCI Chess Engines
Designed by Fabien Letouzey and Huang Chen("Morning Yellow")
Version: 1.3W, Last Modified: Jan. 2006

PolyGlot 1.3W is the modified version of 1.3 by Fabien Letouzey,
which can be compiled and run under Windows without CygWin.

The version of 1.3W has the following modifications:
1. Added a module, "pipe.h/pipe.cpp".
2. Removed two modules, "io.h/io.cpp" and "engine.h/engine.cpp".
3. Modified all statements related on these modules.
4. fixed "Move Now" (the bug fixed in PolyGlot 1.4).

PolyGlot 1.3W can be downloaded from:
http://www.elephantbase.net/download/polyglot.rar
For information of "PolyGlot 1.3", see "readme.txt"
For legal information, see "copying.txt"
