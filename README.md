# t64fix

**Version 0.4.0**

## New in version 0.4.0

* Add `--create` command: create a T64 image and write one or more PRG files to it.
* Have the option parser properly report a missing argument for string arguments,
  not just fail silently.


## Introduction

t64fix is a small tool to fix faulty T64 tape images. T64 images are container
files used by the old C64S emulator by Miha Peternel. The 'tape' in the name is
a misnomer as these files have nothing to do with actual tape dumps, they're
used as archives and for storing memory snapshots in C64S.

There are a lot of faulty t64 files out there, mostly created with tools which
were written by people who didn't really understand the format.


## Usage

Usage is relatively simple: `t64fix [options] <SOURCE> [<DESTINATION>]`

When just checking if a t64 file is correct, just issue `t64fix <SOURCE>`. This
will verify the image and show its contents and any warnings on stdout. To fix
an image, issue `t64fix <SOURCE> <DESTINATION>`, this will verify and fix
\<SOURCE\> and write it to \<DESTINATION\>. Using the same for SOURCE and
DESTINATION is fine, SOURCE is read into memory and then closed, so using
`t64fix foo.t64 foo.t64` will fix foo.t64 and write it back.


More 'advanced' use is available through a few command line switches:

| Option                                  | Description                                         |
| --------------------------------------- | ----------------------------------------------------|
| -q/--quiet                              | don't output anything to stdout, for use in scripts |
| -e/--extract \<index\>                  | extract file \<index\> from image                   |
| -x/--extract-all                        | extract all files, except memory snapshots          |
| -c/--create \<image\> \<list-of-files\> | create t64 image and write on or more files to it   |
| --help                                  | show help                                           |
| --version                               | show version info                                   |


The `--quiet` option tells t64fix to not output any information on stdout, it
just returns an exit code (`EXIT_SUCCESS` or `EXIT_FAILURE`). See the bash
script `scripts/verify_multi.sh` for an example.



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


## Future

I probably won't be adding any more features to this tool, unless specifically
asked for them. Any bugs found I will fix, and of course accept patches for
them.
Adding support for exporting to d64 is something I thought about and have
decided to leave that to a library I'm writing. Better to write it once properly
than multiple times half-assed.


### TODO

- Set a name for the image created by the `--create` command, either by using
  the image's basename as a source or as an optional argument to the command.
  (Currently the image's basename excluding extension is used)

### BUGS

- Tapename created needs to be converted to PETSCII.
- Filenames need to be padded with spaces ($20).



