import struct

import Code.VarGen as VarGen

if VarGen.isWindows:
    import win32api, win32process, win32con
else:
    import psutil

NORMAL, LOW, HIGH, VERYHIGH, VERYLOW = range(5)

def setPriority(pid, tipo):
    if VarGen.isWindows:
        # Based on:
        # http://stackoverflow.com/questions/1023038/change-process-priority-in-python-cross-platform
        # "Recipe 496767: Set Process Priority In Windows" on ActiveState
        #   http://code.activestate.com/recipes/496767/
        setPriorityWindows(pid, tipo)
    else:
        setPriorityLinux(pid, tipo)

def setPriorityQProcess(qprocess, tipo):
    # typedef struct _PROCESS_INFORMATION {
    # HANDLE hProcess;
    # HANDLE hThread;
    # DWORD  dwProcessId;
    # DWORD  dwThreadId;
    # } PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
    if tipo:  # El tipo normal no se va a usar
        if VarGen.isWindows:
            hp, ht, pid, dt = struct.unpack("PPII", qprocess.pid().asstring(16))
            setPriorityWindows(int(pid), tipo)
        else:
            pid = qprocess.pid()
            if pid:
                setPriorityLinux(pid, tipo)

def setPriorityWindows(pid, tipo):
    dic = {VERYLOW: win32process.IDLE_PRIORITY_CLASS,
           LOW: win32process.BELOW_NORMAL_PRIORITY_CLASS,
           HIGH: win32process.ABOVE_NORMAL_PRIORITY_CLASS,
           VERYHIGH: win32process.HIGH_PRIORITY_CLASS}
    handle = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, True, pid)
    win32process.SetPriorityClass(handle, dic[tipo])

def setPriorityLinux(pid, tipo):
    dic = {VERYLOW: 20,
           LOW: 10,
           HIGH: -10,
           VERYHIGH: -20}
    p = psutil.Process(int(pid))
    p.set_nice(dic[tipo])

