# vim: set not ts=8
VPATH=src
CC=gcc
LD=gcc

INSTALL_PREFIX=/usr/local

# -Wformat-signedness not in GCC < 5.0??
CFLAGS=-Wall -Wextra -pedantic -std=c89 -Wshadow -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
	-Wswitch-default -Wswitch-enum -Wuninitialized -Wconversion \
	-Wredundant-decls -Wnested-externs -Wunreachable-code \
	-g -O3

OBJS = main.o d64.o t64.o base.o
HEADERS = base.h d64.h t64.h

TARGET=t64fix
DOCS=doc


all: $(TARGET)

.PHONY: doc
doc:
	doxygen 1>/dev/null

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rfd $(DOCS)/html/*
	rm -f *.html

install:
	cp $(TARGET) $(INSTALL_PREFIX)/bin


# generic rule to build objects from sourcefiles
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(LD) -o $(TARGET) $(OBJS)
