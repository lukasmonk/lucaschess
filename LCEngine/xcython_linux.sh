#!/usr/bin/env bash
rm LCEngine.so

x=$(pwd)
export LIBRARY_PATH=$x
export LD_LIBRARY_PATH=$x
export PATH=$x:$PATH
python ./setup.py build_ext --inplace --verbose


if uname -m | grep 64
then
	folder=64
else
	folder=32
fi
cp LCEngine.so ../Engines/Linux$folder/_tools
cp libirina.so ../Engines/Linux$folder/_tools
