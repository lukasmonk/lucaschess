del *.pyd
SET DISTUTILS_USE_SDK=1

SET MSSdk=1

set VISUALCFORPYTHON=%USERPROFILE%\AppData\Local\Programs\Common\Microsoft\Visual C++ for Python\9.0
set VCINSTALLDIR=%VISUALCFORPYTHON%\VC
set WindowsSdkDir=%VISUALCFORPYTHON%\WinSDK
set PATH=%VCINSTALLDIR%\Bin;%WindowsSdkDir%\Bin;%PATH%
set INCLUDE=%VCINSTALLDIR%\Include;%WindowsSdkDir%\Include;%INCLUDE%
set LIB=%VCINSTALLDIR%\Lib;%WindowsSdkDir%\Lib;%LIB%
set LIBPATH=%VCINSTALLDIR%\Lib;%WindowsSdkDir%\Lib;%LIBPATH%



python setup.py build_ext --inplace
copy LCEngine4.pyd ..\Engines\Windows\_tools

