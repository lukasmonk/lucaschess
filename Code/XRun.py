import subprocess
import sys

def run_lucas(*args):
    li = []
    if sys.argv[0].endswith(".py"):
        li.append("pythonw.exe")
        li.append("./Lucas.py")
    else:
        li.append("Lucas.exe")
    li.extend(args)
    return subprocess.Popen(li)
