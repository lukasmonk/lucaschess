CPP = g++
CPPFLAGS = -DNDEBUG -Wall -O3 -std=c++11 -msse -msse3 -mpopcnt -pthread

BIN = Demolito.exe

OBJ = bitboard.o \
		eval.o \
		gen.o \
		magic.o \
		main.o \
		move.o \
		position.o \
		pst.o \
		search.o \
		sort.o \
		test.o \
		tt.o \
		tune.o \
		types.o \
		uci.o \
		zobrist.o

LINKOBJ = $(OBJ)

.PHONY: all clean

all: $(BIN)

clean:
	rm -f $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -lpthread -static -o $(BIN)

.o: .cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@
