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

/** @file   d64.c - d64 support
 *
 * @brief   Simple d64 write support to be able to convert t64 to d64
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "base.h"

#include "d64.h"

#define ASSERT_TRACK(T) assert((T) >= D64_TRACK_MIN && (T) <= D64_TRACK_MAX)

typedef struct trk_info_s {
    size_t  offset;
    int     sectors;
} trk_info_t;


static trk_info_t track_table[D64_TRACK_COUNT] = {
    /* tracks 1-17 */
    { 0x00000, 21 }, { 0x01500, 21 }, { 0x02a00, 21 }, { 0x03f00, 21 },
    { 0x05400, 21 }, { 0x06900, 21 }, { 0x07e00, 21 }, { 0x09300, 21 },
    { 0x0a800, 21 }, { 0x0bd00, 21 }, { 0x0d200, 21 }, { 0x0e700, 21 },
    { 0x0fc00, 21 }, { 0x11100, 21 }, { 0x12600, 21 }, { 0x13b00, 21 },
    { 0x15000, 21 },
    /* tracks 18-24 */
    { 0x16500, 19 }, { 0x17800, 19 }, { 0x18b00, 19 }, { 0x19e00, 19 },
    { 0x1b100, 19 }, { 0x1c400, 19 }, { 0x1d700, 19 },
    /* tracks 25-30 */
    { 0x1ea00, 18 }, { 0x1fc00, 18 }, { 0x20e00, 18 }, { 0x22000, 18 },
    { 0x23200, 18 }, { 0x24400, 18 },
    /* tracks 31-35 */
    { 0x25600, 17 }, { 0x26700, 17 }, { 0x27800, 17 }, { 0x28900, 17 }
};



static size_t d64_track_offset(int track)
{
    ASSERT_TRACK(track);
    return track_table[track - 1].offset;
}


static int d64_track_sector_count(int track)
{
    ASSERT_TRACK(track);
    return track_table[track - 1].sectors;
}



static void d64_bam_init(d64_image_t *image)
{
    unsigned char *bam = image->data + D64_BAM_OFFSET;

    /* set directory track and sector */
    bam[D64_BAM_DIR_TRACK] = D64_DIR_TRACK;
    bam[D64_BAM_DIR_SECTOR] = D64_DIR_SECTOR;
    bam[D64_BAM_DOS_VERSION] = 0x41;
    /* set disk name, id and some unused bytes to 0xA0 */
    memset(bam + D64_BAM_DISK_NAME, 0xab - 0x90, 0xa0);
    /* set DOS type to '2A' */
    bam[D64_BAM_DOS_TYPE + 0] = 0x32;
    bam[D64_BAM_DOS_TYPE + 1] = 0x41;
}



/** @brief  Set disk name
 *
 * Set disk name to \a name, using at most 16 characters
 *
 * @param   image   d64 image
 * @param   name    disk name
 */
void d64_set_disk_name(d64_image_t *image, const char *name)
{
    strncpy((char *)(image->data + D64_BAM_OFFSET + D64_BAM_DISK_NAME),
            name, D64_BAM_DISK_NAME_LEN);
}


/** @brief  Set disk ID
 *
 * Set disk ID to \a id, using at most 5 characters.
 *
 * @param   image   d64 image
 * @param   id      disk ID
 */
void d64_set_disk_id(d64_image_t *image, const char *id)
{
    strncpy((char *)(image->data + D64_BAM_OFFSET + D64_BAM_DISK_ID),
            id, D64_BAM_DISK_ID_LEN);
}





/** @brief  Create new d64 image
 *
 * Create a new, 'formatted', d64 image.
 *
 * @return  d64 image or NULL on failure
 */
d64_image_t *d64_new(void)
{
    d64_image_t *image = malloc(sizeof *image);
    if (image == NULL) {
        base_err_alloc(sizeof *image);
        return NULL;
    }
    image->path = NULL;
    image->data = calloc(D64_SIZE, 1);
    if (image->data == NULL) {
        base_err_alloc(D64_SIZE);
        free(image);
        return NULL;
    }
    d64_bam_init(image);
    return image;
}




