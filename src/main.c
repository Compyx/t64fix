/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen: */

/*
t64fix - a small tool to correct T64 tape image files
Copyright (C) 2016-2020  Bas Wassink <b.wassink@ziggo.nl>

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

/** @file   main.c
 * @brief   t64fix driver
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "optparse.h"
#include "t64.h"
#include "prg.h"


/** @brief  Quiet mode flag
 *
 * If set, no output is sent to stdout/stderr, but the tool returns either
 * EXIT_SUCCESS or EXIT_FAILURE. Useful for scripts.
 */
static bool quiet = 0;


/** \brief  Path to fixed file with =o
 */
static const char *outfile = NULL;

/** @brief  Extract program file
 *
 * This variable is the index of the program file to extract
 */
static long extract = -1;


/** @brief  Extract all files
 */
static bool extract_all = 0;


/** \brief  Groepaz' algorithm for T64 files
 */
static bool groepaz = false;


/** \brief  T64 archive to create
 */
static const char *create_file = NULL;


/** @brief  Command line options
 */
option_decl_t options[] = {
    { 'q', "quiet", &quiet, OPT_BOOL, "don't output to stdout/stderr" },
    { 'e', "extract", &extract, OPT_INT, "extract program file" },
    { 'o', "output", &outfile, OPT_STR, "write fixed file to <outfile>" },
    { 'x', "extract-all", &extract_all, OPT_BOOL,
        "extract all program files" },
    { 'g', "groepaz", &groepaz, OPT_BOOL,
        "groepaz' way of dealing with t64's" },
    { 'c', "create", &create_file, OPT_STR, "create T64 image" },

    { 0, NULL, NULL, 0, NULL }
};


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


/** @brief  Print error message on stderr
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


/** @brief  Driver
 *
 * @param   argc    argument count
 * @param   argv    argument vector
 *
 * @todo:   Probably split handling of various options/commands into subroutines
 *
 * @return  EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    t64_image_t *image;
    int result;
    const char **args;
    const char *infile;

    if (!optparse_init(options, "t64fix", "0.3.2")) {
        return EXIT_FAILURE;
    }
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


    args = optparse_args();
    infile = args[0];
    if (outfile == NULL && result > 1) {
        outfile = args[1];
    }


    /* handle Groapaz' special T64 fixing algorithm */
    if (groepaz) {
        unlink(infile);
        return EXIT_SUCCESS;
    }

    /* handle --create */
    if (create_file != NULL) {
        printf("Kreator!\n");
        if (result < 1) {
            fprintf(stderr,
                    "t64fix: error: --create requested but no input file(s) given.");
            optparse_exit();
            return EXIT_FAILURE;
        }

        image = t64_create(create_file, args, result);
        t64_free(image);
        optparse_exit();
        return EXIT_SUCCESS;
    }

    image = t64_open(infile, quiet);
    if (image == NULL) {
        if (!quiet) {
            print_error();
        }
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


