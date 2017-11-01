import subprocess
import sys

from Code import VarGen


def run_lucas(*args):
    li = []
    iswindows = VarGen.isWindows
    if sys.argv[0].endswith(".py"):
        li.append("pythonw.exe" if iswindows else "python")
        li.append("./Lucas.py")
    else:
        li.append("Lucas.exe" if iswindows else "./Lucas")
    li.extend(args)
    return subprocess.Popen(li)
