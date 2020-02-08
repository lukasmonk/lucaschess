#!/bin/bash
###  modify as appropriate for you system
### all builds have added features, 4 opening books can be used, adaptive ply,
### play by FIDE Elo ratings or CCRL Elo ratings
###

### time the compile process
set echo on
#DATE=$(shell date +"%m/%d/%y")
start=`date +%s`

#ARCH="ARCH=general-32"
#ARCH="ARCH=x86-32-old"
#ARCH="ARCH=x86-32"
#ARCH="ARCH=general-64"
#ARCH="ARCH=x86-64"
ARCH="ARCH=x86-64-modern"
#ARCH="ARCH=x86-64-bmi2"
#ARCH="ARCH=armv7"
#ARCH="ARCH=ppc-32"
#ARCH="ARCH=ppc-64comp"

#COMP="COMP=clang"
COMP="COMP=gcc"
#COMP="COMP=mingw"
#COMP="COMP=icc"

#BUILD="build"
BUILD="profile-build"

#make function
function mke() {
make -j $BUILD $ARCH $COMP "$@"
}

mke NOIR=yes && wait
mke BLUEFISH=yes FORTRESS_DETECT=yes &&
mke BLUEFISH=yes &&
mke HONEY=yes BLUEFISH=yes FORTRESS_DETECT=yes &&
mke HONEY=yes BLUEFISH=yes &&
mke HONEY=yes FORTRESS_DETECT=yes &&
mke HONEY=yes &&
mke WEAKFISH=yes &&
mke FORTRESS_DETECT=yes &&
mke

### The script code belows computes the bench nodes for each version, and updates the Makefile
### with the bench nodes and the date this was run.
echo ""
mv benchnodes.txt benchnodes_old.txt
echo "$( date +'Based on commits through %m/%d/%Y:')">> benchnodes.txt
echo "======================================================">> benchnodes.txt
grep -E 'searched|Nodes/second' *.bench  /dev/null >> benchnodes.txt
echo "======================================================">> benchnodes.txt
sed -i -e  's/^/### /g' benchnodes.txt
#rm *.nodes benchnodes.txt-e
echo "$(<benchnodes.txt)"
sed -i.bak -e '30,52d' ../src/Makefile
sed '29r benchnodes.txt' <../src/Makefile >../src/Makefile.tmp
mv ../src/Makefile.tmp ../src/Makefile


end=`date +%s`
runtime=$((end-start))
echo ""
echo Processing time $runtime seconds...
