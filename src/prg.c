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

/** @file   prg.c *
 * @brief   Extracting PRG files
 *
 * Functions to extract program (*.prg) files from t64 images. 'Frozen' files
 * ('frz') are skipped, these are snapshots of the C64S emulator and nobody
 * in their right mind would use that emulator today.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "base.h"
#include "t64.h"

#include "prg.h"


/** @brief  Extract prg file at \a index from \a image
 *
 * @param   image   t64 image
 * @param   index   index in \a image of file to extract
 * @param   quiet   don't output anything to stdout/stderr
 *
 * @todo:   Needs proper PETSCII to ASCII translation
 *
 * @return  bool
 */
bool prg_extract(const t64_image_t *image, int index, int quiet)
{
    t64_record_t *record;
    char name[T64_REC_FILENAME_LEN + 5];    /* +4 for '.prg', + 1 for 0 */
    int i;


    if (index < 0 || index >= image->rec_used) {
        t64_errno = T64_ERR_INDEX;
        return false;
    }

    record = image->records + index;

    /* check for memory snapshot */
    if (record->c64s_ftype > 1 || record->c1541_ftype == 0x00) {
        if (!quiet) {
            fprintf(stderr, "t64fix: skipping memory snapshot\n");
        }
        return true;
    }
    /* translate filename */
    for (i = 0; i < T64_REC_FILENAME_LEN; i++) {
        int ch = record->filename[i];
        name[i] = isprint(ch) && ch != '/' ? (char)ch : '_';
    }
    /* remove padding */
    i--;
    while (i >= 0 && name[i] == 0x20) {
        name[i--] = '\0';
    }
    strcpy(name + i + 1, ".prg");
    if (!quiet) {
        printf("t64fix: writing prg file '%s'\n", name);
    }
    return fwrite_prg(name, image->data + record->offset,
            (size_t)(record->real_end_addr - record->start_addr),
            record->start_addr);
}


/** @brief  Extract all files
 *
 * @param   image   t64 image
 * @param   quiet   don't output information on stdout/stderr
 *
 * @return  bool
 */
bool prg_extract_all(const t64_image_t *image, int quiet)
{
    int i;
    int skipped = 0;

    for (i = 0; i < image->rec_used; i++) {
        t64_record_t *record = image->records + i;
        if (record->c64s_ftype > 1 || record->c1541_ftype == 0x00) {
            if (!quiet) {
                printf("t64fix: skipping file %d: memory snapshot\n", i);
            }
            skipped++;
        } else {
            if (!prg_extract(image, i, quiet)) {
                return false;
            }
        }
    }
    if (!quiet) {
        printf("t64fix: extracted %d files\n", image->rec_used - skipped);
    }
    return true;
}

