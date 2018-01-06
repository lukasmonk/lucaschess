#!/usr/bin/python2.7
# -*- coding: utf-8 -*-

# ==============================================================================
# Author : Lucas Monge, lukasmonk@gmail.com
# Web : http://lucaschess.pythonanywhere.com/
# Blog : http://lucaschess.blogspot.com
# Licence : GPL
# ==============================================================================

import os
import sip
from imp import reload
import sys

import argparse
import logging

def str2bool(v):
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

def chessFile(v):
    filetypes = "(pgn|pks|lcg|lcf|bmt)"
    import re
    if not re.match(".*\." + filetypes, v.lower()):
        raise argparse.ArgumentTypeError(
            "Unkown filetype, please provide either: %s", filetypes)
    return v

parser = argparse.ArgumentParser(description='Process command line arguments.')
parser = argparse.ArgumentParser()

parser.add_argument("-l", "--log", dest="logLevel",
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the logging level")

parser.add_argument("-kibitzer", type=str2bool, nargs='?',
                        const=True, default=False,
                        help="Activate kibitzer.")

parser.add_argument("-play", dest="fen",
                        help="Play a fen")

parser.add_argument("chessFile", type=chessFile, nargs='?',
                        help="Chess file to open.")

args = parser.parse_args()

if args.logLevel:
    logging.basicConfig(level=getattr(logging, args.logLevel))

logging.info("Args: %s", args)

kibitzer = args.kibitzer

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

current_dir = os.path.dirname(sys.argv[0])
if current_dir:
    os.chdir(current_dir)

import Code.VarGen
sys.path.append(os.path.join(current_dir, "Code"))
sys.path.append(os.path.join(current_dir, Code.VarGen.folder_engines, "_tools"))

import Code.Traducir as Traducir
Traducir.install()

#TODO -- clean up sys.argv usage, use argparser, this is a hack, improve to use proper args
argArr = sys.argv
sys.argv = [x for x in argArr if x.endswith(".py")]


if kibitzer:
    import Code.RunKibitzer
    Code.RunKibitzer.run(args.chessFile)
else:
    import Code.Init
    Code.Init.init(args)
