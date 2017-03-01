@echo off
call "c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;D:\Program Files\LLVM\bin;%PATH%
clang -O2 -DWIN32 -D_CRT_SECURE_NO_WARNINGS main.c loop.c board.c data.c util.c movegen.c makemove.c perft.c eval.c evalst.c search.c person.c hash.c book.c log.c -o ..\irina.exe
strip ..\irina.exe
