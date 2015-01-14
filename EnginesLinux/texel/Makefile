SRC	= bitBoard.cpp book.cpp computerPlayer.cpp enginecontrol.cpp evaluate.cpp \
	  game.cpp history.cpp humanPlayer.cpp killerTable.cpp kpkTable.cpp \
	  krkpTable.cpp krpkrTable.cpp material.cpp move.cpp moveGen.cpp parameters.cpp \
	  parallel.cpp piece.cpp position.cpp search.cpp texel.cpp textio.cpp \
	  transpositionTable.cpp treeLogger.cpp tuigame.cpp uciprotocol.cpp \
	  util/logger.cpp util/random.cpp util/timeUtil.cpp util/util.cpp

CXX		= g++
CXXFLAGS	= -Wall -std=c++11 -pthread -DHAVE_CTZ -DHAVE_POPCNT -DHAVE_PREFETCH
LDFLAGS		= -pthread -lrt
STRIP		= strip
OBJS_32		= $(patsubst %.cpp,obj32/%.o,$(SRC))
OBJS_64		= $(patsubst %.cpp,obj64/%.o,$(SRC))


CXX_WIN 	= /home/petero/mingw64/bin/x86_64-w64-mingw32-g++
CXXFLAGS_WIN	= -O3 -Wall -std=c++11 -pthread -march=corei7 -DHAVE_CTZ -DHAVE_POPCNT -DHAVE_PREFETCH
LDFLAGS_WIN	= -pthread
STRIP_WIN	= /home/petero/mingw64/bin/x86_64-w64-mingw32-strip
OBJS_W64 	= $(patsubst %.cpp,objw64/%.o,$(SRC))

CXX_WINAMD 	= /home/petero/mingw64/bin/x86_64-w64-mingw32-g++
CXXFLAGS_WINAMD	= -O3 -Wall -std=c++11 -pthread -march=athlon64-sse3 -mpopcnt -DHAVE_CTZ -DHAVE_POPCNT -DHAVE_PREFETCH
LDFLAGS_WINAMD	= -pthread
STRIP_WINAMD	= /home/petero/mingw64/bin/x86_64-w64-mingw32-strip
OBJS_W32AMD 	= $(patsubst %.cpp,objw32amd/%.o,$(SRC))
OBJS_W64AMD 	= $(patsubst %.cpp,objw64amd/%.o,$(SRC))

CXX_WINOLD 	= /home/petero/mingw64/bin/x86_64-w64-mingw32-g++
CXXFLAGS_WINOLD	= -O3 -Wall -std=c++11 -pthread
LDFLAGS_WINOLD	= -pthread
STRIP_WINOLD	= /home/petero/mingw64/bin/x86_64-w64-mingw32-strip
OBJS_W32OLD 	= $(patsubst %.cpp,objw32old/%.o,$(SRC))
OBJS_W64OLD 	= $(patsubst %.cpp,objw64old/%.o,$(SRC))

CXX_WIN32 	= /home/petero/mingw32/bin/i686-w64-mingw32-g++
CXXFLAGS_WIN32	= -O3 -Wall -std=c++11 -pthread -march=athlon64-sse3 -mpopcnt -DHAVE_CTZ -DHAVE_POPCNT -DHAVE_PREFETCH
LDFLAGS_WIN32	= -pthread
STRIP_WIN32	= /home/petero/mingw32/bin/i686-w64-mingw32-strip
OBJS_W32 	= $(patsubst %.cpp,objw32/%.o,$(SRC))

CXX_WIN32OLD 	= /home/petero/mingw32/bin/i686-w64-mingw32-g++
CXXFLAGS_WIN32OLD = -O3 -Wall -std=c++11 -pthread
LDFLAGS_WIN32OLD = -pthread
STRIP_WIN32OLD	= /home/petero/mingw32/bin/i686-w64-mingw32-strip
OBJS_W32OLD 	= $(patsubst %.cpp,objw32old/%.o,$(SRC))


CXX_DEF		= g++
CXXFLAGS_DEF	= -O3 -Wall -std=c++11 -pthread
LDFLAGS_DEF	= -pthread -lrt
STRIP_DEF	= strip
OBJS_DEF	= $(patsubst %.cpp,objdef/%.o,$(SRC))


CXX_ARM 	= /home/petero/androidgcc/bin/arm-linux-androideabi-g++ -D__STDC_INT64__ \
	          -D_GLIBCXX_USE_C99_STDINT_TR1 -D_GLIBCXX_HAS_GTHREADS -D_GLIBCXX__PTHREADS -D_POSIX_TIMEOUTS
CXXFLAGS_ARM	= -O3 -Wall -std=c++11 -pthread -mthumb -march=armv7-a -DHAVE_CTZ
LDFLAGS_ARM	= -pthread -mthumb -march=armv7-a
STRIP_ARM 	= /home/petero/androidgcc/bin/arm-linux-androideabi-strip
OBJS_ARM 	= $(patsubst %.cpp,objarm/%.o,$(SRC))


ALL_EXE = texel64 texel-arm texel64.exe texel64amd.exe texel64old.exe texel32.exe texel32old.exe

OBJS    = $(OBJS_32) $(OBJS_64) $(OBJS_ARM) $(OBJS_W64) $(OBJS_W64AMD) $(OBJS_W64OLD) \
          $(OBJS_W32) $(OBJS_W32OLD) $(OBJDEF)


default : texel
all	: $(ALL_EXE)

strip   : FORCE $(ALL_EXE)
	$(STRIP) texel64
	$(STRIP_WIN) texel64.exe
	$(STRIP_WINAMD) texel64amd.exe
	$(STRIP_WINOLD) texel64old.exe
	$(STRIP_WIN32) texel32.exe
	$(STRIP_WIN32OLD) texel32old.exe
	$(STRIP_ARM) texel-arm

dist	: texel.7z

texel.7z: FORCE $(ALL_EXE) strip
	(VER=$$(echo -e 'uci\nquit' | ./texel64 | grep 'id name' | awk '{print $$4}' | tr -d .) ; \
	 rm -f texel$${VER}.7z ; \
	 7za a texel$${VER}.7z Makefile src/*.?pp src/util/*.?pp $(ALL_EXE))


$(OBJS_DEF) : objdef/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_DEF) $(CXXFLAGS_DEF) -m64 -c -o $@ $<

texel	 : $(OBJS_DEF) Makefile
	$(CXX_DEF) $(LDFLAGS_DEF) -m64 -o $@ $(OBJS_DEF)



$(OBJS_64) : obj64/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) -m64 -c -o $@ $<

texel64	 : $(OBJS_64) Makefile
	$(CXX) $(LDFLAGS) -m64 -o $@ $(OBJS_64)

$(OBJS_32) : obj32/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) -m32 -c -o $@ $<

texel32  : $(OBJS_32) Makefile
	$(CXX) $(LDFLAGS) -m32 -o $@ $(OBJS_32)



$(OBJS_W64) : objw64/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_WIN) $(CXXFLAGS_WIN) -m64 -c -o $@ $<

texel64.exe : $(OBJS_W64) Makefile
	$(CXX_WIN) $(LDFLAGS_WIN) -m64 -o $@ $(OBJS_W64) -static

$(OBJS_W64AMD) : objw64amd/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_WINAMD) $(CXXFLAGS_WINAMD) -m64 -c -o $@ $<

texel64amd.exe : $(OBJS_W64AMD) Makefile
	$(CXX_WINAMD) $(LDFLAGS_WINAMD) -m64 -o $@ $(OBJS_W64AMD) -static

$(OBJS_W64OLD) : objw64old/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_WINOLD) $(CXXFLAGS_WINOLD) -m64 -c -o $@ $<

texel64old.exe : $(OBJS_W64OLD) Makefile
	$(CXX_WINOLD) $(LDFLAGS_WINOLD) -m64 -o $@ $(OBJS_W64OLD) -static


$(OBJS_W32) : objw32/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_WIN32) $(CXXFLAGS_WIN32) -m32 -c -o $@ $<

texel32.exe : $(OBJS_W32) Makefile
	$(CXX_WIN32) $(LDFLAGS_WIN32) -m32 -o $@ $(OBJS_W32) -static

$(OBJS_W32OLD) : objw32old/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_WIN32OLD) $(CXXFLAGS_WIN32OLD) -m32 -c -o $@ $<

texel32old.exe : $(OBJS_W32OLD) Makefile
	$(CXX_WIN32OLD) $(LDFLAGS_WIN32OLD) -m32 -o $@ $(OBJS_W32OLD) -static


$(OBJS_ARM) : objarm/%.o : src/%.cpp
	@mkdir -p $$(dirname $@)
	$(CXX_ARM) $(CXXFLAGS_ARM) -c -o $@ $<

texel-arm  : $(OBJS_ARM) Makefile
	$(CXX_ARM) $(LDFLAGS_ARM) -o $@ $(OBJS_ARM) -static


clean 	:
	rm -rf $(OBJS) *~ obj32 obj64 objarm objw64 objw64amd objw64old objw32 objw32old objdef

.PHONY	: clean dist FORCE
