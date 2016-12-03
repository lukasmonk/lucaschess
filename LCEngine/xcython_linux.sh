rm LCEngine.so
export LIBRARY_PATH=<to change>/pyDBgames/LCEngine
export LD_LIBRARY_PATH=<to change>/pyDBgames/LCEngine
export PATH=<to change>/pyDBgames/LCEngine:$PATH
python setup.py build_ext --inplace --verbose

cp LCEngine.so ..
cp libirina.so ..
