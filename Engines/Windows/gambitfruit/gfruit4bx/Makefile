
# files

EXE = gfruit

OBJS = attack.o board.o book.o eval.o fen.o hash.o list.o main.o material.o \
       move.o move_check.o move_do.o move_evasion.o move_gen.o move_legal.o \
       option.o pawn.o piece.o posix.o protocol.o pst.o pv.o random.o \
       search.o search_full.o see.o sort.o square.o trans.o util.o value.o \
       vector.o

# rules

all: $(EXE) .depend

clean:
	$(RM) *.o .depend gmon.out

# general

CXX      = clang
CXXFLAGS = -pipe
LDFLAGS  = -lm

# C++

CXXFLAGS += -fno-exceptions -fno-rtti

# optimisation

CXXFLAGS += -O2 -fstrict-aliasing
CXXFLAGS += -fomit-frame-pointer
# CXXFLAGS += -march=athlon-xp # SELECT ME

# strip

LDFLAGS += -s

# dependencies

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

.depend:
	$(CXX) -MM $(OBJS:.o=.cpp) > $@

include .depend

