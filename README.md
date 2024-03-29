# t64fix v0.4.0

## New in version 0.4.0

- Add `--create` command: create a T64 image and write one or more PRG files to it.
- Have the option parser properly report a missing argument for string arguments,
  not just fail silently.
- Improve `make install`
- Add man page.


## Introduction

t64fix is a small tool to fix faulty T64 tape images. T64 images are container
files used by the old C64S emulator by Miha Peternel. The 'tape' in the name is
a misnomer as these files have nothing to do with actual tape dumps, they're
used as archives and for storing memory snapshots in C64S.

There are a lot of faulty t64 files out there, mostly created with tools which
were written by people who didn't really understand the format.


## Usage

Usage is relatively simple: `t64fix [OPTIONS] <SOURCE>`

When just checking if a t64 file is correct, just issue `t64fix <SOURCE>`. This
will verify the image and show its contents and any warnings on stdout. To fix
an image, issue `t64fix <SOURCE> -o <DESTINATION>`, this will verify and fix
\<SOURCE\> and write it to \<DESTINATION\>. Using the same for SOURCE and
DESTINATION is fine, SOURCE is read into memory and then closed, so using
`t64fix foo.t64 -o foo.t64` will fix foo.t64 and write it back. But obviously
the original file will be lost, so this is not advisable.


More 'advanced' use is available through a few command line switches:

| Option                                    | Description                                         |
|:----------------------------------------- |:----------------------------------------------------|
| `-q, --quiet`                             | don't output anything to stdout, for use in scripts |
| `-o, --output <fixed-image>`              | write fixed image to filesystem                     |
| `-e, --extract <index>`                   | extract file \<index\> from image                   |
| `-x, --extract-all`                       | extract all files, except memory snapshots          |
| `-c, --create <image> <list-of-files>`    | create t64 image and write on or more files to it   |
| `--help`                                  | show help                                           |
| `--version`                               | show version info                                   |


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


## Building and installing

To build t64fix simply run `make`. To install run `make install` as root.

Targets for `make`:

| Target          | Result
|:--------------- |:------------------------------------------------------------------------- |
| \<none\>        | Build t64fix without debugging enabled                                    |
| `all`           | Build t64fix without debugging enabled                                    |
| `clean`         | Remove t64fix and all intermediate objects, delete Doxygen documentation  |
| `debug`         | Build t64fix with debugging enabled                                       |
| `dist`          | Generate distribution tarball (`t64fix-$(VERSION).tar.gz`)                |
| `doc`           | Generate Doxygen documentation (in `doc/html`)                            |
| `install`       | Install t64fix executable and its man page                                |
| `install-bin`   | Install t64fix executable                                                 |
| `install-man`   | Install t64fix man page                                                   |
| `windist`       | Generate Windows distribution zipfile (`t64fix-win[32|64]-$(VERSION).zip` |



## Future

I probably won't be adding any more features to this tool, unless specifically
asked for them. Any bugs found I will fix, and of course accept patches for
them.

### TODO

- d64 support


### BUGS

none(?)


## Resources

- [t64fix GitHub project page](https://github.com/Compyx/t64fix/)
- [t64fix website](https://compyx.github.io/t64fix/)
- [T64 file format description](http://unusedino.de/ec64/technical/formats/t64.html)
