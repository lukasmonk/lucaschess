# -*- coding: utf-8 -*-


import os
import sys


reload(sys)
sys.setdefaultencoding("latin-1")
sys.path.append(os.curdir)

import sip

sip.setapi('QDate', 2)
sip.setapi('QDateTime', 2)
sip.setapi('QString', 2)
sip.setapi('QTextStream', 2)
sip.setapi('QTime', 2)
sip.setapi('QUrl', 2)
sip.setapi('QVariant', 2)

import Code.Traducir as Traducir
Traducir.install()

current_dir = os.path.dirname(sys.argv[0])
if current_dir:
    os.chdir(current_dir)

nArgs = len(sys.argv)
if nArgs == 1:
    import Code.Init
    Code.Init.init()

elif nArgs >= 2:
    arg = sys.argv[1].lower()
    if arg.endswith(".pgn") or arg.endswith(".pks") \
            or arg.endswith(".lcg") or arg.endswith(".lcf") \
            or arg == "-play":
        import Code.Init
        Code.Init.init()

    elif arg == "-sound":
        import Code.RunSound
        Code.RunSound.run(sys.argv[2])

    elif arg == "-kibitzer":
        import Code.RunKibitzer
        Code.RunKibitzer.run(sys.argv[2])

    elif arg == "-voyager":
        import Code.RunVoyager
        Code.RunVoyager.run(sys.argv[2])


