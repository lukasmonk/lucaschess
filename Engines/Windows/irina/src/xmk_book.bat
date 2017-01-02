@echo off
call "c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;D:\Program Files\LLVM\bin;%PATH%
clang -O2 -DWIN32 -D_CRT_SECURE_NO_WARNINGS book.c -o b.exe
b
