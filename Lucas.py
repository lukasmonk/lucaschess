#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

# ==============================================================================
# Author : Lucas Monge, lukasmonk@gmail.com
# Web : http://www-lucaschess.rhcloud.com
# Blog : http://lucaschess.blogspot.com/
# Licence : GPL
# ==============================================================================



"""
1. Mate label in analysis. (dima d)
2. When program path includes cyrillic characters. (tserv)
3. Merging polyglot books.
4. Renaming a Personal opening guide. (Mario L)
5. Changed stockfish with a compilation valid to old pcs. (Jörg R)
6. Play like a GM, when saving to PGN. (Uli)
7. Cinnamon problems working with low fixed depths, detected but not fixed. (Xema)
"""

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
            or arg == "-play" or arg.endswith(".bmt"):
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


