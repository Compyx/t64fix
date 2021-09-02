# vim: set noet ts=8 sw=8 sts=8:
VPATH=src
CC=gcc
LD=$(CC)

# Program (Msys2 automagically adds .exe)
TARGET=t64fix
# Version (used for `t64fix --help` and `make dist`)
VERSION=0.4.0-rc1

# Installation prefix for `make install`
PREFIX ?= /usr/local

MAN_PATH = $(PREFIX)/share/man/man1


# These flags are know to work with GCC >= 8.3.0
#
# Add -DDEBUG to enable debugging messages
#
CFLAGS=-W -Wall -Wextra -pedantic -std=c99 -Wshadow -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
	-Wswitch-default -Wswitch-enum -Wuninitialized -Wconversion \
	-Wredundant-decls -Wnested-externs -Wunreachable-code -Wuninitialized \
	-Wdiscarded-qualifiers -Wsign-compare -DVERSION=\"$(VERSION)\" -g -O3


# Object files
OBJS = main.o base.o cbmdos.o optparse.o petasc.o prg.o t64.o



# Files for `make dist`
DIST_FILES = \
	CHANGES.md \
	COPYING \
	Doxyfile \
	Makefile \
	README.md \
	doc/man/t64fix.1 \
	scripts/verify_multi.sh \
	src/base.c \
	src/base.h \
	src/cbmdos.c \
	src/cbmdos.h \
	src/main.c \
	src/optparse.c \
	src/optparse.h \
	src/petasc.c \
	src/petasc.h \
	src/prg.c \
	src/prg.h \
	src/t64.c \
	src/t64.h \
	src/t64types.h \

WINDIST_FILES = \
	CHANGES.md \
	COPYING \
	README.md


# Distribution temp directory
DIST_DIR=t64fix-$(VERSION)
# Distrubition tarball
DIST_TGZ=$(DIST_DIR).tar.gz

# Windows distribution temp directory
ifeq ($(MSYSTEM_CARCH),x86_64)
	WINDIST_DIR := t64fix-win64-$(VERSION)
endif
ifeq ($(MSYSTEM_CARCH),i686)
	WINDIST_DIR := t64fix-win32-$(VERSION)
endif
WINDIST_ZIP := $(WINDIST_DIR).zip


# Target for `make [all]`
all: $(TARGET)

# dependencies of objects
base.o:
cbmdos.o:
main.o: base.o optparse.o prg.o t64.o t64types.h
optparse.o:
petasc.o:
prg.o: base.o petasc.o t64types.h
t64.o: base.o cbmdos.o petasc.o


debug: CPPFLAGS=-DDEBUG
debug: $(TARGET)


.PHONY: doc
doc:
	doxygen 1>/dev/null

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe
	rm -rfd doc/html/*
	rm -f *.html
	if [ -d $(DIST_DIR) ]; then \
	    rm -rd $(DIST_DIR); \
	fi
	if [ -f $(DIST_TGZ) ]; then \
	    rm $(DIST_TGZ); \
	fi
	if [ "x$(WINDIST_DIR)" != "x" -a -d $(WINDIST_DIR) ]; then \
	    rm -rd $(WINDIST_DIR); \
	fi
	if [ "x$(WINDIST_DIR)" != "x" -a -f $(WINDIST_ZIP) ]; then \
	    rm $(WINDIST_ZIP); \
	fi


install-bin: $(TARGET)
	install -s -m 755 -g root -o root $(TARGET) $(PREFIX)/bin

install-man: doc/man/t64fix.1
	install -g root -o root -d $(MAN_PATH)
	install -m 755 -g root -o root doc/man/t64fix.1 $(MAN_PATH)
	gzip -f $(MAN_PATH)/t64fix.1


install: install-bin install-man


# Create source distribution tarball
dist: $(DIST_FILES)
	if [ -d $(DIST_DIR) ]; \
	    then rm -rd $(DIST_DIR); \
	fi
	if [ -f $(DIST_TGZ) ]; then \
	    rm $(DIST_TGZ); \
	fi
	mkdir $(DIST_DIR)
	cp -a --parents $(DIST_FILES) $(DIST_DIR)
	tar -czf $(DIST_TGZ) $(DIST_DIR)
	rm -rd $(DIST_DIR)


# Create Windows distribution zipfile
windist: $(TARGET) $(DIST_FILES)
	if [ -d $(WINDIST_DIR) ]; then \
	    rm -rd $(WINDIST_DIR); \
	fi
	if [ -f $(WINDIST_ZIP) ]; then \
	    rm $(WINDIST_ZIP); \
	fi
	mkdir $(WINDIST_DIR)
	cp -a --parents $(WINDIST_FILES) $(WINDIST_DIR)
	cp -a $(TARGET).exe $(WINDIST_DIR)
	strip $(WINDIST_DIR)/$(TARGET).exe
	zip $(WINDIST_ZIP) $(WINDIST_DIR)/*
	rm -rd $(WINDIST_DIR)


# generic rule to build objects from source files
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(LD) -o $(TARGET) $(OBJS)
