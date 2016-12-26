rm LCEngine.so

x=$(pwd)
export LIBRARY_PATH=$x
export LD_LIBRARY_PATH=$x
export PATH=$x:$PATH
python ./setup.py build_ext --inplace --verbose

cp LCEngine.so ..
cp libirina.so ..
