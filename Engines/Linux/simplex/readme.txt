Simplex is an engine who use the UCI protocol to comunicate with the GUI.

Simplex 0.9.6

- Simplex now use performance counters if available. 
  Now shorter time control is possible.
- Simplified quiesce function.
- Nullmove R=3, not two in a row.
- Evaluation only on the tips.
- New evaluation from scratch. Dumb, strange, stronger?
- removed the KpK endgame bitbase.
- corrected some bugs, noticeably in the hash move validation.
- added age in the trasposition table.
- move order changed.


Simplex 0.9.3
This is the first Simplex delivered with sources.
Some of the code can be shared with my other engine Rocinante.

I have recovered some sources, dated in May 2008, under the name DarkSimplex 0.9.3
from experiments about the published description of DarkThought,
 Markus Gille and Ernst Heinz available in the Web.(http://people.csail.mit.edu/heinz/dt/)

I am not sure that this engine is better than the previous one: Simplex (2007),
but it will be the basis for future developments.

To do list:
- prepare the code for multiprocessor execution.
- improve the evaluation.
- add new bitbases.

Changes so far:
- renamed the engine to its original name Simplex.
- Separated all sources and artifacts of debugging and experimentation.
- Auto tuning (poor results).
- Manual tuning.
- I made some test to verify the stability of the engine.
- porting to Linux (Open Suse 64 bits).


This engine compiles on windows with compilers Microsoft, GNU (Mingw).(Intel not tested).
In linux(64-bit) I have tried  the GNU and the Intel compiler.

Windows System

Visual C
cl src/*.cpp src/win/*.cpp -o simplex.exe

Gnu C
g++ src/*.cpp src/win/sistema.cpp  -o simplex

Linux...

g++ src/*.cpp src/posix/sistema.cpp  -o simplex
