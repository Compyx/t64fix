/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen : */

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

/** @file   t64.h
 * @brief   Header for t64.c
 */


#ifndef HAVE_T64_H
#define HAVE_T64_H

#include "base.h"

#define T64_HDR_MAGIC       0x00    /**< magic 'C64*', unreliable */
#define T64_HDR_MAGIC_LEN   0x20    /**< length of magic bytes */
#define T64_HDR_VERSION     0x20    /**< tape version, 16-bit le word */
#define T64_HDR_REC_MAX     0x22    /**< max number of records, byte */
#define T64_HDR_REC_USED    0x24    /**< current number of records, byte */
#define T64_HDR_NAME        0x28    /**< name of the tape, PETSCII, padded with
                                         0x20 */
#define T64_HDR_NAME_LEN    0x18    /**< length of the tape name */


#define T64_RECORDS_OFFSET  0x40    /**< offset in container of records */

#define T64_RECORD_SIZE     0x20    /**< size of a file record */

#define T64_REC_C64S_FILETYPE   0x00    /**< C64S file type */
#define T64_REC_C1541_FILETYPE  0x01    /**< C1541 file type */
#define T64_REC_START_ADDR      0x02    /**< start address of file, 16-bit le */
#define T64_REC_END_ADDR        0x04    /**< end address of file, 16-bit le */
#define T64_REC_CONTENTS        0x08    /**< offset in container to file data,
                                             32-bit little endian */
#define T64_REC_FILENAME        0x10    /**< PETSCII filename */

#define T64_REC_FILENAME_LEN    0x10    /**< maximum length of a filename */


/** @brief  Enum indicating the status of a record
 */
typedef enum {
    T64_REC_OK,         /**< record is OK */
    T64_REC_FIXED,      /**< record was fixed */
    T64_REC_SKIPPED     /**< record was skipped (frozen files) */
} t64_status_t;



/** @brief  t64 file record type
 *
 * Contains information of a single file in the container
 */
typedef struct t64_record_s {
    unsigned char   filename[16];   /**< filename in PETSCII */
    unsigned long   offset;         /**< offset in container of file data */
    unsigned short  start_addr;     /**< start address on C64 */
    unsigned short  end_addr;       /**< end address on C64 (exclusive) */
    unsigned short  real_end_addr;  /**< real end address after fixing */
    unsigned char   c64s_ftype;     /**< C64S file type */
    unsigned char   c1541_ftype;    /**< C1541 file type */
    int             index;          /**< index in container records */
    t64_status_t    status;         /**< record status (OK, fixed, skipped) */
} t64_record_t;



/** @brief  t64 container type
 */
typedef struct t64_image_s {
    unsigned char   magic[T64_HDR_MAGIC_LEN];   /**< tape magic in ASCII */
    unsigned char   tapename[T64_HDR_NAME_LEN]; /**< name of tape in PESTCII */
    const char *    path;           /**< path to container file */
    unsigned char * data;           /**< container file data */
    size_t          size;           /**< size of data */
    t64_record_t *  records;        /**< file records */
    unsigned short  rec_max;        /**< maximum number of records */
    unsigned short  rec_used;       /**< current number of records */
    unsigned short  version;        /**< tape version */
    int             fixes;          /**< number of fixes applied */
} t64_image_t;



t64_image_t *   t64_open(const char *path, int quiet);
void            t64_free(t64_image_t *image);
int             t64_verify(t64_image_t *image, int quiet);
void            t64_dump(const t64_image_t *image);
bool            t64_write(t64_image_t *image, const char *path);

#endif



