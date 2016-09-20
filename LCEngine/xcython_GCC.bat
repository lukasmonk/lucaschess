PATH=c:\mingw\bin;%PATH%
set LIBRARY_PATH=.
del *.pyd
REM python setup.py build_ext --inplace
python setup.py build_ext --inplace --compiler=mingw32
del ..\LCEngine.pyd
copy LCEngine.pyd ..

