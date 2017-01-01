cheng4 0.38
------------
by Martin Sedlak
opening book line by Graham Banks
independent testing by Lars Hallerstrom, Silvian Rucsandescu and Fonzy Bluemers
lazy SMP idea (improved shared hashtable) by Dan Homan (motivated by lazy Julien Marcel)
lockless hashing idea by Harry Nelson/Bob Hyatt
hosting by Emil Vlasak
eval tuning idea by Peter Osterlung (many thanks), also known as "Texel tuning method"
logos by Radovan Kramar, Graham Banks, Jim Ablett and Silvian Rucsandescu
greetings to all CC friends (list would be too long)

supports FRC, UCI/xboard mode (but no ini!)
strength (0.38): my guess is +30 elo lower bound

note for Linux/BSD/OSX users: may need to run chmod u+x on the binary first

fixes in 0.35a:
- ponder fix (actually fixed command event race problems)
- more verbose now (so adjudication will now work for very fast games)

new in 0.36:
- fix book probing
- add knight outpost eval
- add knbk endgame eval
- minor eval weight tuning
- safer futility margins
- better history heuristic
- multipv support (may need some testing)
- experimental strength limit
- PV no longer extracted from hash table (you get real PV now)
- fix upperbound reporting in UCI mode
- report selective depth now (doesn't include qs nodes)
- limit minimum qs depth to avoid search explosions in artificial positions
- autodetect hardware popcount
=> estimated elo gain: +28.5

fixes in 0.36a:
- FRC fix (castling when opponent slider on back rank) (thanks to Ray Banks)
- analyzing checkmate returns null move (Arena analysis fix)
- CECP st command fix (seconds instead of centiseconds)

0.36c:
- many fixes
- minor improvements (+5 elo)
- going open source (despite unpolished and some things unfinished)
- Android compile
- XP 64-bit fix
- bench should be 3413675
- this is the last version, I'm done with cheng

0.38:
- safe mobility
- extensive eval tuning (using Peter's method) (rouhly 80% of improvement is due to this)
- retune delta margin
- play single reply asap
- don't start new iteration if it's unlikely that there's enough time
  to search first move
- fix kbnk after tuning
- rewrite root move sorting (zero impact on elo)
- bench should read 5616320 nodes
- various minor fixes and changes
- don't spam the GUI in first second(s) of thinking

4.39:
- better versioning now: Cheng 4.39
- good history moves reduced anyway at higher depths
- added table-based mobility
- good/bad bishop knowledge
- final retuning using Texel tuning method
- minor bugfixes (including CECP sd command)
- root moves refactored
- pin-aware mobility (probably just my imagination that it works)
- better passer scoring
- get rid of nullmove verification search and nullthreat code (now using hybrid nmp-nmr)
- lower razor margins a bit
- bench-tune IID depths
- bench is now searching to d15 instead of d13, should be 10025265

have fun
