LINK_TARGET = ../libirina.a

OBJS = board.o data.o eval.o hash.o loop.o makemove.o movegen.o movegen_piece_to.o search.o test.o util.o pgn.o

REBUILDABLES = $(OBJS) $(LINK_TARGET)

clean :
	rm -f $(REBUILDABLES)
	echo Clean done

all : $(LINK_TARGET)
	echo All done

$(LINK_TARGET) : $(OBJS)
	ar rcs $(LINK_TARGET) $^


%.o : %.c
	gcc -Wall -O3 -o $@ -c $< -DNDEBUG
	#~ mas rapida y menor tam = -O1
	#~ profile mas -pg
	#~ clang -Wall -O2 -o $@ -c $<
