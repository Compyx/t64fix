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

/** \brief  Path to fixed file with =o
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
 */
static void help_prologue(void)
{
    printf("Examples:\n\n");
    printf("  Inspect T64 file for errors:\n");
    printf("    t64fix <input>\n");
    printf("  Fix t64 file and save as new file:\n");
    printf("    t64fix <input> -o <output>\n");
    printf("  Extract all files as .PRG files:\n");
    printf("    t64fix -x <input>\n");
    printf("  Create T64 archive:\n");
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
    t64_image_t *image;
    const char **args;
    const char *infile;
    int result;

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
    infile = args[0];
    if (outfile == NULL && result > 1) {
        /* support `t64fix <infile> <outfile>` syntax */
        outfile = args[1];
    }

    /* handle --create command */
    if (create_file != NULL) {
        if (result < 1) {
            fprintf(stderr,
                    "t64fix: error: `--create` requested but no input file(s) "
                    "given.\n");
            optparse_exit();
            return EXIT_FAILURE;
        }

        image = t64_create(create_file, args, result, quiet);
        if (image != NULL) {
            if (!t64_write(image, create_file)) {
                fprintf(stderr,
                        "t64fix: error: failed to write image '%s'\n",
                        create_file);
                print_error();
                t64_free(image);
                optparse_exit();
                return EXIT_FAILURE;
            }
            t64_free(image);
        }
        optparse_exit();
        return EXIT_SUCCESS;
    }

    /* no --create switch given, verify given image */
    image = t64_open(infile, quiet);
    if (image == NULL) {
        if (!quiet) {
            print_error();
        }
        optparse_exit();
        return EXIT_FAILURE;
    }
    /* verify image and display results */
    result = t64_verify(image, quiet);
    if (!quiet) {
        t64_dump(image);
    }

    /* write corrected image if requested */
    if (outfile != NULL) {
        if (!quiet) {
            printf("Writing corrected image to '%s' .. ", outfile);
            fflush(stdout);
        }
        if (t64_write(image, outfile)) {
            if (!quiet) {
                printf("OK\n");
            }
        } else {
            if (!quiet) {
                printf("failed\n");
            }
        }
    }


    if (extract_all) {
        if (!prg_extract_all(image, quiet)) {
            print_error();
        }
    } else if (extract >= 0) {
        if (!prg_extract(image, (int)extract, quiet)) {
            print_error();
        }
    }

    /* clean up */
    t64_free(image);
    optparse_exit();

    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}


