/** \file   base.h
 * \brief   Memory allocation, I/O, string handling, error messages - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

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

#ifndef BASE_H_
#define BASE_H_

#ifdef DEBUG
# include <stdio.h>
# include <stdarg.h>
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/** \brief Error codes
 */
typedef enum t64_error_code_t {
    T64_ERR_NONE,               /**< no error */
    T64_ERR_OOM,                /**< out-of-memory error */
    T64_ERR_IO,                 /**< I/O error, inspect `errno` for details */
    T64_ERR_T64_INVALID,        /**< not a T64 image */
    T64_ERR_INDEX,              /**< invalid index */
    T64_ERR_D64_TRACK_RANGE,    /**< d64 track number out of range */
    T64_ERR_D64_SECTOR_RANGE,   /**< d64 sector number out of range */
    T64_ERR_D64_INVALID_FILENAME,   /**< d64 invalid filename */
    T64_ERR_D64_RLE             /**< d64 RLE error */
} T64ErrorCode;


/** \brief  Minimum valid error code
 */
#define T64_ERRNO_MIN   0

/** \brief  Maximum valid error code
 */
#define T64_ERRNO_MAX   T64_ERR_INDEX


/** \def    base_debug
 *\brief    Print debug message
 *
 * Print debug message on stdout if DEBUG is defined.
 */
#ifdef DEBUG
# define base_debug(...) \
    printf("[debug] %s:%d::%s(): ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    putchar('\n');
#else
# define base_debug(...)
#endif



extern int      t64_errno;


uint16_t        get_uint16(const uint8_t *p);
void            set_uint16(uint8_t *p, uint16_t v);
uint32_t        get_uint32(const uint8_t *p);
void            set_uint32(uint8_t *p, uint32_t v);
unsigned int    num_blocks(unsigned int n);
int             popcount_byte(uint8_t b);

long            fread_alloc(uint8_t **dest, const char *path);

bool            fwrite_wrapper(const char *path, const uint8_t *data,
                               size_t size);
bool            fwrite_prg(const char *path, const uint8_t *data,
                           size_t size, int start);
const char *    t64_strerror(int code);


void *          base_malloc(size_t size);
void *          base_calloc(size_t nmemb, size_t size);
void *          base_realloc(void *ptr, size_t size);
void            base_free(void *ptr);

char *          base_strdup(const char *s);
const char *    base_basename(const char *path, const char **ext);

void            base_hexdump(const uint8_t *src, size_t len, size_t voffset);

#endif

