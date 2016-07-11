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

/** @file   base.c
 *
 * @brief   Base functions
 */

#ifndef BASE_H_
#define BASE_H_

#include <stdlib.h>



/** @brief  Boolean type
 */
typedef enum { false, true } bool;


/** @brief Error codes
 */
typedef enum t64_error_code_t {
    T64_ERR_NONE,       /**< no error */
    T64_ERR_OOM,        /**< out-of-memory error */
    T64_ERR_IO,         /**< I/O error, inspect `errno` for details */
    T64_ERR_INVALID,    /**< not a T64 image */
    T64_ERR_INDEX       /**< invalid index */
} T64ErrorCode;


/** @brief  Minimum valid error code
 */
#define T64_ERRNO_MIN   0


/** @brief  Maximum valid error code
 */
#define T64_ERRNO_MAX   T64_ERR_INVALID



extern int t64_errno;

int             base_err_alloc(size_t n);
unsigned short  get_uint16(const unsigned char *p);
void            set_uint16(unsigned char *p, unsigned short v);
unsigned long   get_uint32(const unsigned char *p);
void            set_uint32(unsigned char *p, unsigned long v);
unsigned int    num_blocks(unsigned int n);

long            fread_alloc(unsigned char **dest, const char *path);

bool            fwrite_wrapper(const char *path, const unsigned char *data,
                               size_t size);
bool            fwrite_prg(const char *path, const unsigned char *data,
                           size_t size, int start);
const char *    t64_strerror(int code);

#endif

