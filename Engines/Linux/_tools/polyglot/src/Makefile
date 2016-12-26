# ####################################################
# Makefile for use with GNU tools
# ####################################################


# Be as portable as possible
SHELL = /bin/sh
.SUFFIXES: .cpp .o


# Executable name
EXE = polyglot


# Compiler, compilation- and linker flags
CXX = g++
CXXFLAGS = -Wall -O3 -fomit-frame-pointer -DNDEBUG
LFLAGS = -s


# Source and object files
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)


# Compilation and dependencies
$(EXE): $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) -o $(EXE)
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $<


# Clean up
clean:
	rm -f $(OBJS) $(EXE) *~
