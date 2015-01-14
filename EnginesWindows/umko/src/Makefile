NAME			= Umko
VERSION			= 0.7
CXXFLAGS		+= -O3 -g0 -s -DNDEBUG -Wall -W -D NAME='"$(NAME)"' -D VERSION='"$(VERSION)"'
LFLAGS			+= -O3 -g0 -s -DNDEBUG -Wl
CXXFLAGS_DEBUG	+= -O0 -g3 -Wall -W -D NAME='"$(NAME)"' -D VERSION='"$(VERSION)"'
LFLAGS_DEBUG	+= -O0 -g3 -Wl
OBJECTS = main.o search.o engine.o types.o \
	book.o trans.o pos_move.o test.o \
	pos_eval.o pos_gen.o pos_init.o pos_core.o \
	movegen.o movelist.o thread.o rootmoves.o \
	timer.o egbb.o
	
OBJECTS_DEBUG = $(addprefix debug/, $(OBJECTS))
OBJECTS_RELEASE = $(addprefix release/, $(OBJECTS))
TEST = wac.epd bwtc.epd wcs.epd ecm.epd bk.epd lctII.epd bt2630.epd bs2830.epd
TEST_DIR = test/`date +"%d_%m_%Y"`
XBOARD_FLAGS=-tc 0:10 -size Large -mg 100 -fd . -sd . -coords -thinking -highlight\
			 -pgnExtendedInfo true -ponderNextMove false\
			 -usePolyglotBook true -polyglotBook ./performance.bin\
			 -fNoOwnBookUCI -sNoOwnBookUCI -lightSquareColor white\
			 -darkSquareColor \\\#7389b6 -whitePieceColor white\
			 -highlightSquareColor blue -debugMode true

all: linux_x64

release: release_dir $(OBJECTS_RELEASE)
	$(CXX) $(LFLAGS) -o $(TARGET) $(OBJECTS_RELEASE) $(OBJCOMP) $(LIBS)

release/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

release_dir:
	mkdir -p release

debug: debug_dir $(OBJECTS_DEBUG)
	$(CXX) $(LFLAGS_DEBUG) -o $(TARGET) $(OBJECTS_DEBUG) $(OBJCOMP) $(LIBS)

debug/%.o : %.cpp
	$(CXX) $(CXXFLAGS_DEBUG) $(DEBUG) -c $< -o $@

debug_dir:
	mkdir -p debug

help:
	@echo release:
	@echo make linux_x64
	@echo make linux_x32
	@echo make windows_x64
	@echo make windows_x32
	@echo test:
	@echo make test_linux_x64
	@echo make test_linux_x32
	@echo make test_windows_x64
	@echo make test_windows_x32
	@echo debug:
	@echo make linux_x64_debug
	@echo make linux_x32_debug
	@echo make windows_x64_debug
	@echo make windows_x32_debug
	@echo help:
	@echo make help
	@echo additionaly for linux_x64:
	@echo make position
	@echo make test_crafty
	@echo make test_bbchess

linux_x32:
	CXX="g++" CXXFLAGS="-m32 -march=i686" \
	LFLAGS="-m32"  LIBS="-L/usr/lib/ -lz -ldl -lpthread" \
	TARGET="umko_x32" make release

linux_x32_debug:
	CXX="g++" CXXFLAGS_DEBUG="-m32 -march=i686" \
	LFLAGS_DEBUG="-m32"  LIBS="-L/usr/lib/ -lz -ldl -lpthread" \
	TARGET="umko_x32_debug" make debug

linux_x64:
	CXX="g++" CXXFLAGS="-m64" \
	LFLAGS="-m64" LIBS="-L/usr/lib64/ -lz -ldl -lpthread" \
	TARGET="umko_x64" make release

linux_x64_debug:
	CXX="g++" CXXFLAGS_DEBUG="-m64" \
	LFLAGS_DEBUG="-m64" LIBS="-L/usr/lib64/ -lz -ldl -lpthread" \
	TARGET="umko_x64_debug" make debug

windows_x32:
	CXX="i586-pc-mingw32-g++" \
	CXXFLAGS="-m32 -march=i686 -static -I./zlib/include/" \
	LFLAGS="-static -m32"  LIBS="./zlib/lib32/libz.a" \
	TARGET="umko_x32.exe" make release

windows_x32_debug:
	CXX="i586-pc-mingw32-g++" \
	CXXFLAGS_DEBUG="-m32 -march=i686 -I./zlib/include/" \
	LFLAGS_DEBUG="-m32"  LIBS="./zlib/lib32/libz.a" \
	TARGET="umko_x32_debug.exe" make debug

windows_x64:
	CXX="x86_64-w64-mingw32-g++" \
	CXXFLAGS="-m64 -static -I./zlib/include/" \
	LFLAGS="-m64 -static"  LIBS="./zlib/lib64/libz.a" \
	TARGET="umko_x64.exe" make release

windows_x64_debug:
	CXX="x86_64-pc-mingw32-gcc" \
	CXXFLAGS_DEBUG="-m64-I./zlib/include64/" \
	LFLAGS_DEBUG="-m64"  LIBS="./zlib/lib64/libz.a" \
	TARGET="umko_x64_debug.exe" make debug

test_linux_x64:
	TARGET="umko_x64" RELEASE="linux_x64" make test

test_linux_x32:
	TARGET="umko_x32" RELEASE="linux_x32" make test

test_windows_x32:
	TARGET="umko_x32.exe" RELEASE="windows_x32" make test

test_windows_x64:
	TARGET="umko_x64.exe" RELEASE="windows_x64" make test

wac.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup umko_x64 -epdtime 5 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

bwtc.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 5 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

wcs.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 5 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

ecm.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 20 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

bk.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 60 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

lctII.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 600 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

bt2630.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 900 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

bs2830.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup ./$(TARGET) -epdtime 900 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

ece3.epd: $(RELEASE)
	mkdir -p $(TEST_DIR)
	nohup umko_x64 -epdtime 5 media/$@.gz $(TEST_DIR)/$@.unsolved.epd $(TEST_DIR)/$@.log > $(TEST_DIR)/$@.out

test: $(TEST)
	@echo Fnished

position: linux_x64
	mkdir -p $(TEST_DIR)
	rm -f ./$(TEST_DIR)/input ./$(TEST_DIR)/output.dat
	mkfifo ./$(TEST_DIR)/input
	tail -f ./$(TEST_DIR)/input | umko_x64 > ./$(TEST_DIR)/output.dat &
	echo "uci" > ./$(TEST_DIR)/input
	echo "isready" > ./$(TEST_DIR)/input
	echo "position fen 1k1r3r/pp2qpp1/3b1n1p/3pNQ2/2pP1P2/2N1P3/PP4PP/1K1RR3 b - - 0 1" > ./$(TEST_DIR)/input
	echo "go infinite" > ./$(TEST_DIR)/input

test_crafty: linux_x64
	mkdir -p $(TEST_DIR)
	xboard $(XBOARD_FLAGS) -fUCI -fcp ./umko_x64 -scp crafty -sgf ./$(TEST_DIR)/crafty_game.pgn -debugFile ./$(TEST_DIR)/crafty.log
	echo -e "readpgn $(TEST_DIR)/crafty_game.pgn\nelo\n\t offset 2600\n\t mm\n\t ratings >$(TEST_DIR)/crafty.rat\n\t x\n\nx\n" | bayeselo > /dev/null 2> /dev/null
	cat $(TEST_DIR)/crafty.rat

test_togaII: linux_x64
	mkdir -p $(TEST_DIR)
	xboard $(XBOARD_FLAGS) -fUCI -sUCI -fcp ./umko_x64 -scp togaII -sgf ./$(TEST_DIR)/togaII_game.pgn -debugFile ./$(TEST_DIR)/togaII.log
	echo -e "readpgn $(TEST_DIR)/togaII_game.pgn\nelo\n\t offset 2600\n\t mm\n\t ratings >$(TEST_DIR)/togaII.rat\n\t x\n\nx\n" | bayeselo > /dev/null 2> /dev/null
	cat $(TEST_DIR)/togaII.rat

test_stockfish: linux_x64
	mkdir -p $(TEST_DIR)
	xboard $(XBOARD_FLAGS) -fUCI -sUCI -fcp ./umko_x64 -scp stockfish -sgf ./$(TEST_DIR)/stockfish_game.pgn  -debugFile ./$(TEST_DIR)/stockfish.log
	echo -e "readpgn $(TEST_DIR)/stockfish_game.pgn\nelo\n\t offset 2600\n\t mm\n\t ratings >$(TEST_DIR)/stockfish.rat\n\t x\n\nx\n" | bayeselo > /dev/null 2> /dev/null
	cat $(TEST_DIR)/stockfish.rat

test_bbchess: linux_x64
	mkdir -p $(TEST_DIR)
	xboard $(XBOARD_FLAGS) -fUCI -fcp ./umko_x64 -sUCI -scp bbchess -sgf ./$(TEST_DIR)/bbchess_game.pgn -debugFile ./$(TEST_DIR)/bbchess.log
	echo -e "readpgn $(TEST_DIR)/bbchess_game.pgn\nelo\n\t offset 2600\n\t mm\n\t ratings >$(TEST_DIR)/bbchess.rat\n\t x\n\nx\n" | bayeselo > /dev/null 2> /dev/null
	cat $(TEST_DIR)/bbchess.rat

archive:
	rm -rf umko_$(VERSION)*
	mkdir umko_$(VERSION)
	mkdir umko_$(VERSION)/w32
	mkdir umko_$(VERSION)/w64
	mkdir umko_$(VERSION)/src
	make clean
	make windows_x32
	cp umko_x32.exe umko_$(VERSION)/w32/
	cp umko.png umko_$(VERSION)/w32/umko_x32.png
	make clean
	make windows_x64
	cp umko_x64.exe umko_$(VERSION)/w64/
	cp umko.png umko_$(VERSION)/w64/umko_x64.png
	cp Copying.txt umko_$(VERSION)/
	cp *.cpp *.h Makefile umko_$(VERSION)/src
	zip -r umko_$(VERSION).zip umko_$(VERSION)
	rm -rf umko_$(VERSION)/
	make clean

clean:
	rm -rf $(OBJECTS_RELEASE) $(OBJECTS_DEBUG) *x64* *x32* 
	rm -rf *~ core *.core input output.dat
	rm -rf game.* log.* polyglot*

