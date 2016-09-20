#makefile to compile Rodent II on Linux

#define the directory for the executable file
BINDIR = /usr/bin

#define the directory for the data files
DATADIR = /usr/share/rodentII


# define the C compiler to use
CC = g++

# define the compile-time flags
CFLAGS = -g -w -Wfatal-errors -pipe -DNDEBUG -O3 -fno-rtti -finline-functions -fprefetch-loop-arrays -DBOOKPATH=$(DATADIR)
C1FLAGS = -g -w -Wfatal-errors -pipe -DWRITEDEBUGFILE -DBOOKPATH=$(DATADIR)

# define the link options
LDFLAGS = -s -lm
LD1FLAGS = -lm

# define outpout name and settings file
EXENAME= rodentII
CONFIGFILE = basic.ini

.PHONY: clean install update remove help

default: build

build:
	@echo "Type make help for additional options"
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(EXENAME) -x c++ compile.linux
	echo "SHOW_OPTIONS" > $(CONFIGFILE)

build-static:
	@echo "Type make help for additional options"
	$(CC) $(LDFLAGS) -static $(CFLAGS) -o $(EXENAME) -x c++ compile.linux
	echo "SHOW_OPTIONS" > $(CONFIGFILE)

build-debug:
	@echo "Type make help for additional options"
	$(CC) $(LD1FLAGS) $(C1FLAGS) -o $(EXENAME) -x c++ compile.linux
	echo "SHOW_OPTIONS" > $(CONFIGFILE)

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXENAME)

install:
	mkdir -p $(BINDIR)
	mkdir -p $(DATADIR)
	cp $(EXENAME) $(BINDIR)
	cp $(CONFIGFILE) $(DATADIR)
	cp ../books/* $(DATADIR)
	cp -r ../personalities/*/*.ini $(DATADIR)
	chmod 755 $(BINDIR)/$(EXENAME)
	chmod 644 $(DATADIR)/*

# this just performs a copy of rodentII engine
update:
	cp $(EXENAME) $(BINDIR)
	
remove:
	rm -f $(BINDIR)/$(EXENAME)
	rm -rf $(DATADIR)
	
help:
	@echo ""
	@echo "To compile RodentII type:"
	@echo ""
	@echo "make build		> Compile Rodent II"
	@echo "make build-static	> Compile Rodent II as a static binary"
	@echo "make build-debug		> Compile Rodent II with Logfile support"
	@echo "make clean 		> Clean up"
	@echo "make install		> Install RodentII (root privileges required)"
	@echo "make update		> Update RodenII engine (root privileges required)"
	@echo "make remove		> Remove RodenII from your system (root privileges required)"
