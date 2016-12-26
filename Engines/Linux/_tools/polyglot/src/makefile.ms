# Makefile for use with Microsoft tools
# > nmake -f makefile.ms

CC = cl
CFLAGS = -nologo -c -O2 -Ox -D "NDEBUG"
LFLAGS = -nologo

# Disable affinity use?  (Needed for Win95 support)
#CFLAGS += -D "NO_AFFINITY"

EXE = polyglot.exe

OBJS = adapter.obj attack.obj board.obj book.obj book_make.obj \
       book_merge.obj colour.obj engine.obj epd.obj fen.obj game.obj \
       hash.obj io.obj line.obj list.obj main.obj move.obj move_do.obj \
       move_gen.obj move_legal.obj option.obj parse.obj pgn.obj \
       piece.obj posix.obj random.obj san.obj search.obj square.obj \
       uci.obj uci_options.obj util.obj

$(EXE): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(EXE)

clean:
	-erase $(OBJS) $(EXE)

.cpp.obj:
	$(CC) $(CFLAGS) $<
