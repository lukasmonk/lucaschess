@echo off

set VISUALCFORPYTHON=c:\Archivos de programa\Archivos comunes\Microsoft\Visual C++ for Python\9.0
set VCINSTALLDIR=%VISUALCFORPYTHON%\VC
set WindowsSdkDir=%VISUALCFORPYTHON%\WinSDK
set PATH=%VCINSTALLDIR%\Bin;%WindowsSdkDir%\Bin;%PATH%
set INCLUDE=%VCINSTALLDIR%\Include;%WindowsSdkDir%\Include;%INCLUDE%
set LIB=%VCINSTALLDIR%\Lib;%WindowsSdkDir%\Lib;%LIB%
set LIBPATH=%VCINSTALLDIR%\Lib;%WindowsSdkDir%\Lib;%LIBPATH%

cl /c /nologo /Ox /MD /GS- /DNDEBUG lc.c board.c data.c eval.c hash.c loop.c makemove.c movegen.c movegen_piece_to.c search.c test.c util.c pgn.c
lib /OUT:..\irina.lib lc.obj board.obj data.obj eval.obj hash.obj loop.obj makemove.obj movegen.obj movegen_piece_to.obj search.obj test.obj util.obj pgn.obj
del *.obj

