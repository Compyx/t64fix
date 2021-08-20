/*
t64fix - a small tool to correct T64 tape image files
Copyright (C) 2016-2021  Bas Wassink <b.wassink@ziggo.nl>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/** \file   main.c
 * \brief   t64fix driver
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "base.h"
#include "optparse.h"
#include "prg.h"
#include "t64types.h"
#include "t64.h"


/** \brief  Quiet mode flag
 *
 * If set, no output is sent to stdout, error message are still sent to stderr.
 *
 *
 */
static bool quiet = 0;

/** \brief  Path to fixed file with -o
 */
static const char *outfile = NULL;

/** \brief  Extract program file
 *
 * This variable is the index of the program file to extract
 */
static long extract = -1;

/** \brief  Extract all files
 */
static bool extract_all = 0;

/** \brief  T64 archive to create
 */
static const char *create_file = NULL;


/** \brief  Command line options
 */
static const option_decl_t options[] = {
    { 'q', "quiet", &quiet, OPT_BOOL,
        "don't output to stdout/stderr" },
    { 'e', "extract", &extract, OPT_INT,
        "extract program file" },
    { 'o', "output", &outfile, OPT_STR,
        "write fixed file to <outfile>" },
    { 'x', "extract-all", &extract_all, OPT_BOOL,
        "extract all program files" },
    { 'c', "create", &create_file, OPT_STR,
        "create T64 image from a list of PRG files" },

    { 0, NULL, NULL, 0, NULL }
};


/** \brief  Print help output prologue
 *
 * Prints some usage examples on stdout.
 */
static void help_prologue(void)
{
    printf("Examples:\n\n");
    printf("  Inspect t64 file for errors:\n");
    printf("    t64fix demos.t64\n");
    printf("  Fix t64 file and save as new file:\n");
    printf("    t64fix demos.t64 -o demos-fixed.t64\n");
    printf("  Extract all files as .PRG files:\n");
    printf("    t64fix -x demos.t64\n");
    printf("  Extract a single .PRG file at index 2:\n");
    printf("    t64fix -e 2 demos.t64\n");
    printf("  Create t64 file:\n");
    printf("    t64fix -c awesome.t64 rasterblast.prg freezer.prg\n");
}


/** \brief  Print error message on stderr
 *
 * If an error of T64_ERR_IO occured, the C library's errno and strerror() is
 * printed as well.
 */
static void print_error(void)
{
    fprintf(stderr, "t64fix: error %d: %s", t64_errno, t64_strerror(t64_errno));
    if (t64_errno == T64_ERR_IO) {
        fprintf(stderr, " (%d: %s)\n", errno, strerror(errno));
    } else {
        putchar('\n');
    }
}


/** \brief  Open image and print error on stderr
 *
 * Open a t64 image and print an error message on stderr on failure.
 *
 * \param[in]   path    path to t64 file
 *
 * \return  t64 image or `NULL` on failure
 */
static t64_image_t *open_image_wrapper(const char *path)
{
    t64_image_t *image = t64_open(path, quiet);

    if (image == NULL) {
        print_error();
    }
    return image;
}


/** \brief  Create t64 file and write .PRG file(s) to it
 *
 * \param[in]   args    list of .PRG files
 * \param[in]   nargs   number of .PRG files in \a args
 *
 * \return  bool
 */
static bool cmd_create(const char **args, int nargs)
{
    t64_image_t *image;

    if (nargs < 1) {
        fprintf(stderr,
                "t64fix: error: `--create` requested but no input file(s) "
                "given.\n");
        return false;
    }

    image = t64_create(create_file, args, nargs, quiet);
    if (image != NULL) {
        if (!t64_write(image, create_file)) {
            fprintf(stderr,
                    "t64fix: error: failed to write image '%s'\n",
                    create_file);
            print_error();
            t64_free(image);
            return false;
        }
        t64_free(image);
    } else {
        fprintf(stderr,
                "t64fix: error: failed to create image.\n");
        print_error();
        return false;
    }
    return true;
}


/** \brief  Verify t64 file, optionally write fixed file
 *
 * Verify t64 file and write fixed file to host when `--outfile` was used.
 *
 * \param[in]   path    path to t64 file
 *
 * \return  true if image OK, false if not OK or when an I/O error occurred
 */
static bool cmd_verify(const char *path)
{
    t64_image_t *image;
    bool status = false;

    /* open image */
    image = open_image_wrapper(path);
    if (image != NULL) {
        /* verify image */
        status = t64_verify(image, quiet);
        if (!quiet) {
            t64_dump(image);
        }

        /* write image to host? */
        if (outfile != NULL) {
            if (!t64_write(image, outfile)) {
                status = false;
                if (!quiet) {
                    print_error();
                }
            }
        }
        t64_free(image);
    }
    return status;
}


/** \brief  Extract a single file from a t64 file
 *
 * \param[in]   path    path to t64 file
 *
 * \return  bool
 */
static bool cmd_extract_indexed(const char *path)
{
    t64_image_t *image;
    bool status = false;

    /* attempt to extract file from image */
    image = open_image_wrapper(path);
    if (image != NULL) {
        /* fix the image quietly so `real_end_addr` is properly set */
        t64_verify(image, true);

        status = prg_extract(image, (int)extract, quiet);
        if (!status) {
            print_error();
        }
        t64_free(image);
    }
    return status;
}


/** \brief  Extract all .PRG files from a t64 file
 *
 * \param[in]   path    path to t64 file
 *
 * \return  bool
 */
static bool cmd_extract_all(const char *path)
{
    t64_image_t *image;
    bool status = false;

    image = open_image_wrapper(path);
    if (image != NULL) {
        /* fix the image quietly so `real_end_addr` is properly set */
        t64_verify(image, true);

        status = prg_extract_all(image, quiet);
        t64_free(image);
    }
    return status;
}



/** \brief  Program driver
 *
 * \param[in]   argc    argument count
 * \param[in]   argv    argument vector
 *
 * \todo    Probably split handling of various options/commands into subroutines
 *
 * \return  EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    const char **args;
    int result;     /* optparse result */
    bool status;    /* command status */

    optparse_init(options, "t64fix", "0.4.0");
    optparse_set_prologue(help_prologue);

    if (argc < 2) {
        /* display help and exit */
        optparse_help();
        optparse_exit();
        return EXIT_FAILURE;
    }

    /* parse command line options */
    result = optparse_exec(argc, argv);
    base_debug("optparse_exec() = %d\n", result);
    if (result == OPT_EXIT_ERROR) {
        optparse_exit();
        return EXIT_FAILURE;
    } else if (result < 0) {
        /* --help or --version */
        optparse_exit();
        return EXIT_SUCCESS;
    } else if (result == 0) {
        fprintf(stderr, "t64fix: no input or output file(s) given, aborting\n");
        optparse_exit();
        return EXIT_FAILURE;
    }

    /* get list of non-option command line args */
    args = optparse_args();

    base_debug("args[0] = '%s'\n", args[0]);

    /* handle commands: */
    if (create_file != NULL) {
        /* --create <outfile> <prg-files> */
        status = cmd_create(args, result);
    } else if (extract >= 0) {
        /* --extract <index> */
        status = cmd_extract_indexed(args[0]);
    } else if (extract_all) {
        /* --extract-all */
        status = cmd_extract_all(args[0]);
    } else {
        /* assume verify */
        status = cmd_verify(args[0]);
    }

    /* clean up */
    optparse_exit();

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
