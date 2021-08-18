# CHANGES

## A list of changes applied to the codebase, started too late as usual ;)


### 2021-08-18

* Update Doxygen docblocks and the Doxyfile.
* Rework optparse to use xmalloc()-style allocation.
* Call abort() rather than exit() on allocation failures to allow debuggers to
  get useful information to work with.


### 2021-08-17

* Remove Windows-specific hacks for %zu and %zx format specifiers.
* Remove Makefile.win, the normal Makefile runs fine in MSys2.
* Remove `-g` command.
* Update README.
* Update copyright lines.


### 2021-08-15

* Finally finish the `--create` command.
* Bump version to 0.4.0


### 2020-07-18

* Update copyright to 2020


### 2018-06-06

* Add the -g/--groepaz command line switch.


### 2016-07-18

* Remove d64 support, too much work. Another project which does support d64
  and other formats will replace this project soon(ish)


### 2016-07-11

* Add extracting program files with -e/--extract \<index\>, or -x/--extract-all
  to extract all program files


### 2016-07-10

* Add simple command line parser (src/optparse.{c,h})
* Add quiet mode (-q/--quiet) for use in scripts


### 2016-07-08

* Add this file
* Start work on D64 support
* Apply patch by iAN CooG: print original magic header, not the fixed one

