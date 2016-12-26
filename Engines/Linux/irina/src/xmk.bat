@echo off
PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
del irina.exe
make all -s
del *.o

