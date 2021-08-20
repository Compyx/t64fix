# vim: set not ts=8
VPATH=src
CC=gcc
LD=gcc

INSTALL_PREFIX=/usr/local

# These flags are know to work with GCC >= 8.3.0
#
# Add -DDEBUG to enable debugging messages
#
CFLAGS=-W -Wall -Wextra -pedantic -std=c99 -Wshadow -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
	-Wswitch-default -Wswitch-enum -Wuninitialized -Wconversion \
	-Wredundant-decls -Wnested-externs -Wunreachable-code -Wuninitialized \
	-Wdiscarded-qualifiers -Wsign-compare \
	-g -O3

OBJS = main.o base.o cbmdos.o optparse.o petasc.o prg.o t64.o
HEADERS = base.h d64.h t64.h optparse.h

TARGET=t64fix
DOCS=doc


all: $(TARGET)

# dependencies of objects
base.o:
cbmdos.o:
main.o: base.o optparse.o prg.o t64.o t64types.h
optparse.o:
petasc.o:
prg.o: base.o t64types.h
t64.o: base.o cbmdos.o petasc.o


.PHONY: doc
doc:
	doxygen 1>/dev/null

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)*
	rm -rfd $(DOCS)/html/*
	rm -f *.html

install:
	cp $(TARGET) $(INSTALL_PREFIX)/bin


# generic rule to build objects from source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(LD) -o $(TARGET) $(OBJS)
