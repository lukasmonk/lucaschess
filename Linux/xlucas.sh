#!/usr/bin/env bash

if uname -m | grep 64
then
	export LD_LIBRARY_PATH=$(pwd)/../Engines/Linux64/_tools
else
	export LD_LIBRARY_PATH=$(pwd)/../Engines/Linux32/_tools
fi
cd ..
python2.7 ./Lucas.py
