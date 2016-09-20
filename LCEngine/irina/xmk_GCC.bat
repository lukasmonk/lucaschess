@echo off
PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
del ..\*.a
make all -s

