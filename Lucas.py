#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

# ==============================================================================
# Author : Lucas Monge, lukasmonk@gmail.com
# Web : http://lucaschess.pythonanywhere.com/
# Blog : http://lucaschess.blogspot.com
# Licence : GPL
# ==============================================================================

# flash-cards




import os
import sip
from imp import reload
import sys

reload(sys)
sys.setdefaultencoding("latin-1")
sys.path.insert(0, os.curdir)

sip.setapi('QDate', 2)
sip.setapi('QDateTime', 2)
sip.setapi('QString', 2)
sip.setapi('QTextStream', 2)
sip.setapi('QTime', 2)
sip.setapi('QUrl', 2)
sip.setapi('QVariant', 2)

current_dir = os.path.abspath(os.path.dirname(sys.argv[0]))
if current_dir:
    os.chdir(current_dir)

from Code import VarGen

sys.path.append(os.path.join(current_dir, "Code"))
sys.path.append(os.path.join(current_dir, VarGen.folder_engines, "_tools"))

import Code.Traducir as Traducir
Traducir.install()

nArgs = len(sys.argv)
if nArgs == 1:
    import Code.Init

    Code.Init.init()

elif nArgs >= 2:
    arg = sys.argv[1].lower()
    if (arg.endswith(".pgn") or arg.endswith(".pks") or
            arg.endswith(".lcg") or arg.endswith(".lcf") or
            arg == "-play" or arg.endswith(".bmt")):
        import Code.Init
        Code.Init.init()

    elif arg == "-kibitzer":
        import Code.RunKibitzer
        Code.RunKibitzer.run(sys.argv[2])

