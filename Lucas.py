#!/usr/bin/python2.7
# -*- coding: utf-8 -*-
# ==============================================================================
# Author : Lucas Monge, lukasmonk@gmail.com
# Web : http://www-lucaschess.rhcloud.com
# Blog : http://lucaschess.blogspot.com/
# Licence : GPL
# ==============================================================================

import os
import sys

reload(sys)
sys.setdefaultencoding("latin-1")
sys.path.append(os.curdir)


import Code.SAK
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
    arg = sys.argv[1]
    argl = arg.lower()
    if argl.endswith(".pgn") or argl.endswith(".pks") \
            or argl.endswith(".lcg") or argl.endswith(".lcf") \
            or argl == "-play":
        import Code.Init
        Code.Init.init()

    elif argl == "-sound":
        import Code.RunSound
        Code.RunSound.run(sys.argv[2])

    elif argl == "-kibitzer":
        import Code.RunKibitzer
        Code.RunKibitzer.run(sys.argv[2])











