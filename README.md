# t64fix

Version 0.1

## Introduction

t64fix is a small tool to fix faulty T64 tape images. T64 images are container
files used by the old C64S emulator by Miha Peternel. The 'tape' in the name is
a misnomer as these files have nothing to do with actual tape dumps, they're
used as archives and for storing memory snapshots in C64S.

There are a lot of faulty t64 files out there, mostly created with tools which
were written by people who didn't really understand the format.


## Usage

Usage is quite simple at the moment: `t64fix SOURCE [DESTINATION]`
If called with only SOURCE, the SOURCE is verified, if called with DESTINATION
the SOURCE is verified, corrected and written to DESTINATION.

### Things that get verified and fixed

There are a few things that get verified and fixed:

#### Header magic string

There are a few different types of header magic strings out there, the correct
one being "C64S image file", padded to 32 bytes with 0x00. This is the string
found in t64 files generated by C64S 2.52.

#### Header tape version number

Version numbers seem to be either 0x100 or 0x101, no information on the
difference can be found, so this tool sets the version number to 0x101, which
is the version number C64S 2.52 uses.

#### Number of file records

Many t64 files have the number of available file records and the number of
used file records wrong, which can lead to tools not correctly reading files
from the container. If either of these numbers is zero, this tools adjusts that
number to one since t64 files with more than one record are rare, and seem to
have their record counts correct.

#### End addresses of files

The thing what makes tools and emulators choke on t64 files. Someone wrote a tool
called CONV64 which incorrectly writes the end address of any file as 0xc3c6.
This causes some tools or emulators to reject the file or silently ignore the
incorrect end address, which causes some decrunchers to fail.

This tool corrects the end address by sorting file records on their data offset
and then using that information, combined with the file size of the t64 file
to determine the correct end address.

#### C1541 file type

Another thing many images get wrong. File records are supposed to have a C1541
file type byte, which normally should be between 0x80 and 0x84. In reality this
is usually something like 0x01 or 0x44. This tool adjusts incorrect values to
0x82 (PRG), since t64 files can really only store PRG files (and C64S' FRZ files)
.




