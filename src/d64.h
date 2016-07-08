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

/** @file   d64.ch- d64 support
 *
 * @brief   Simple d64 write support to be able to convert t64 to d64
 *
 * Only 35-track images supported, D64 images are very cheap, unlike real
 * floppies, so no need for 40-track images.
 */

#ifndef HAVE_D64_H
#define HAVE_D64_H


#define D64_SIZE        0x2ab00     /**< size of a standard 35-track image */


#define D64_TRACK_COUNT 35

#define D64_TRACK_MIN   1
#define D64_TRACK_MAX   35



#define D64_DIR_TRACK   18
#define D64_DIR_SECTOR  1

#define D64_BAM_OFFSET  0x16500     /**< offset in image of BAM */


#define D64_BAM_DIR_TRACK   0x00
#define D64_BAM_DIR_SECTOR  0x01
#define D64_BAM_DOS_VERSION 0x02
#define D64_BAM_ENTRIES     0x04

#define D64_BAM_DISK_NAME   0x90
#define D64_BAM_DISK_NAME_LEN   0x10

#define D64_BAM_DISK_ID     0xa2
#define D64_BAM_DISK_ID_LEN 0x05

#define D64_BAM_DOS_TYPE    0xa5    /**< DOS type, usually '2A' */


/** @brief  D64 image file
 *
 */
typedef struct d64_image_s {
    const char *    path;   /**< path to file */
    unsigned char * data;   /**< file content */
} d64_image_t;


d64_image_t *d64_new(void);

void d64_set_disk_name(d64_image_t *image, const char *name);
void d64_set_disk_id(d64_image_t *image, const char *id);


#endif

