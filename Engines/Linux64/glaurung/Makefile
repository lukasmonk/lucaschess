CC = g++

CFLAGS = -O3 -g -pthread -funroll-loops -march=athlon64 -fomit-frame-pointer -w -flto -fwhole-program 
#CFLAGS = -O2 -funroll-loops -fomit-frame-pointer -Wall -g -mcpu=G5 -mpowerpc64

DEFINES =
LDFLAGS = 

OBJ = benchmark.o bitbase.o bitboard.o book.o color.o direction.o \
	endgame.o evaluate.o history.o main.o material.o mersenne.o misc.o \
	move.o movegen.o movepick.o pawns.o piece.o position.o san.o search.o \
	square.o tt.o uci.o ucioption.o value.o
	
glaurung: .o $(OBJ)
	$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $(OBJ) -o glaurung -lm

.o/%.o: Makefile %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $*.c -o .o/$*.o

.o:
	mkdir .o

clean:
	rm -rf .o glaurung
