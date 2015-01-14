PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
make profile-build ARCH=x86-32 COMP=mingw
pause
strip stockfish.exe
