LINK_TARGET = ../winglet.so

OBJS =  \
	lc.o \
	bitops.o \
	board.o \
	command.o \
	data.o \
	displaymove.o \
	eval.o \
	hash.o \
	makemove.o \
	move.o \
	movegen.o \
	peek.o \
	perft.o \
	qsearch.o \
	readfen.o \
	search.o \
	see.o \
	test.o \
	setup.o \
	sortmoves.o \
	timer.o

REBUILDABLES = $(OBJS) $(LINK_TARGET)

clean :
	rm -f $(REBUILDABLES)
	echo Clean done

all : $(LINK_TARGET)
	echo All done

$(LINK_TARGET) : $(OBJS)
	g++ -shared -O3 -o $@ $^

%.o : %.cpp
	g++ -g -O3 -o $@ -c $<

