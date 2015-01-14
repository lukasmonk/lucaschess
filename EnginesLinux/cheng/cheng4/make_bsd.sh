clang++ -Wall -W -O3 -fno-stack-protector -fno-rtti -fno-exceptions -fomit-frame-pointer -DNDEBUG -U_FORTIFY_SOURCE allinone.cpp -o cheng4_bsd_x64 -lpthread
strip cheng4_bsd_x64
