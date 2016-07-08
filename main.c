/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen : */

/*
t64fix - a small tool to correct T64 tape image files
Copyright (C) 2016  Bas Wassink <b.wassink@ziggo.nl>

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

#include "t64.h"


/** @brief  Print usage message on stdout
 */
static void usage(void)
{
    printf("Usage: t64fix SOURCE [DESTINATION]\n\n");
    printf("If only SOURCE is specified the image will be verified.\n");
    printf("If both SOURCE and DESTINATION are specified, SOURCE will be"
            " verified, fixed\nand written to DESTINATION.\n");
}


/** @brief  Driver
 *
 * @param   argc    argument count
 * @param   argv    argument vector
 *
 * @return  EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        /* display help */
        usage();
    }
    if (argc >= 2) {
#if 0
        printf("Reading '%s' .. ", argv[1]);
        fflush(stdout);
#endif
        t64_image_t *image = t64_open(argv[1]);
        if (image == NULL) {
            printf("failed to load '%s'\n", argv[1]);
            return EXIT_FAILURE;
        }
        /* verify image and display results */
        t64_verify(image);
        t64_dump(image);

        /* write corrected image if requested */
        if (argc > 2) {
            printf("Writing corrected image to '%s' .. ", argv[2]);
            fflush(stdout);
            if (t64_write(image, argv[2])) {
                printf("OK\n");
            } else {
                printf("failed\n");
            }
        }
        /* clean up */
        t64_free(image);
    }

    return EXIT_SUCCESS;
}

