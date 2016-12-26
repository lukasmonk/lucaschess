clang++ -Wall -Wpedantic -W -O3 -std=c++0x -fno-stack-protector -fno-rtti -fno-exceptions -fomit-frame-pointer -DNDEBUG -U_FORTIFY_SOURCE -static allinone.cpp -o cheng4_bsd_x64 -lpthread
strip cheng4_bsd_x64
