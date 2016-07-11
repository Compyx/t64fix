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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "base.h"
#include "t64.h"

#include "prg.h"

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





