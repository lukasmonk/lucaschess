ANDSCACS
========
http://www.andscacs.com/

Andscacs is a chess engine. It's in alpha stage, in initial state of developement, so use at your own risk! I don't offer any warranties.

Is done by Daniel Jose Queralto cdani @ yahoo.com.

Is an UCI engine, so it requires an uci capable user interface to use it.
For example can be used with the free Arena:
http://www.playwitharena.com/

or Chessbase
http://chessbase-shop.com/

I wish to thank many people that publish information about chess engines programming in Internet, especially http://chessprogramming.wikispaces.com/, http://talkchess.com!
Also make special mention of open source modules that are very instructive, as for example Stockfish, Crafty, Protector, Discocheck, Gull...


Versions
========
You have five versions in zip file:
* andscacsb.exe for 64 bit Windows, requires a Cpu with BMI2 support for the instruction PEXT and is the faster one: (https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets#BMI2_.28Bit_Manipulation_Instruction_Set_2.29)
* andscacs.exe for 64 bit Windows, requires a Cpu with support for POPCNT instruction and is a little faster than the next two: (http://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets).
* andscacsn.exe for 64 bit Windows, has not special requirements.
* andscacs32.exe for 32 bit windows. Is something like 50 elo points weaker.
* linux/andscacs, a x64 popcnt Linux version.


Installation examples
=====================

First put andscacs.exe in any folder you want.

In arena, go to "Engines" -> "Install new engine". Search for andscacs.exe. Select Uci as type of engine.

In Chessbase, start a new game, goto "Engines" -> "Create uci engine".
With the "..." button, search for andscacs.exe.


Other thanks
============
Is able to increase its performance on more than 64 cores on Windows thanks to Texel and Stockfish:
https://github.com/official-stockfish/Stockfish/commit/0d9a9f5e985c13852cf9f29767e95f295bb29575
