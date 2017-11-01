W="-Wall -Wextra -pedantic -Wshadow"

echo "building linux compiles"
g++ ./src/*.cc -o ./bin/${1}_sse2   -DNDEBUG -std=c++11 -O3 -msse2   -fno-rtti -flto -s $W
g++ ./src/*.cc -o ./bin/${1}_sse4.2 -DNDEBUG -std=c++11 -O3 -msse4.2 -fno-rtti -flto -s $W

echo "building windows compiles"
x86_64-w64-mingw32-g++ ./src/*.cc -o ./bin/${1}_sse2.exe   -DNDEBUG -std=c++0x -O3 -msse2   -fno-rtti -s -static -flto $W
x86_64-w64-mingw32-g++ ./src/*.cc -o ./bin/${1}_sse4.2.exe -DNDEBUG -std=c++0x -O3 -msse4.2 -fno-rtti -s -static -flto $W

echo "make tarball and cleanup"
cd ./bin
tar czf ./${1}.tar.gz ./${1}_sse*
rm ./${1}_sse*
cd ..
