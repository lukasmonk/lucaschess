
# files

EXE = cyrano

OBJS = board.o debug.o egbbdll.o endgame.o engine.o ethread.o eval.o gen.o genmagic.o hash.o magicmoves.o makemove.o moves.o newbook.o pawns.o prot_e.o prot_i.o random.o search.o see.o static.o think.o uci.o util.o winboard.o winlayer.o
SRC_DIR = ../engine/
SRC_NAMES = $(OBJS:.o=.cpp)
SRCS = $(addprefix $(SRC_DIR),$(SRC_NAMES))


# rules

all: $(EXE) ##.depend

clean:
	$(RM) *.o .depend gmon.out $(EXE)

# general

CXX      = g++
CXXFLAGS = -pipe
LDFLAGS  = -lm

# C++

CXXFLAGS += -W

#  -fwhole-program
# optimisation

CXXFLAGS += -fno-exceptions -fno-rtti
#CXXFLAGS += -O3 -fstrict-aliasing

CXXFLAGS += -O3
# CXXFLAGS += -fomit-frame-pointer
CXXFLAGS += -march=pentium4
# CXXFLAGS += -march=athlon-xp # SELECT ME

# strip

##LDFLAGS += -s -lpthread
LDFLAGS += -lpthread

# dependencies


%.o : $(SRC_DIR)%.cpp
	$(CXX) $(CXXFLAGS) -c $? -o $@

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

##.depend:
##	$(CXX) -MM $(SRCS) > $@

##include .depend

