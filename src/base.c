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

/** @file   base.c
 *
 * @brief   Base functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "base.h"


/** @brief  Block size for fread_alloc()
 *
 * This size is used to allocate memory in fread_alloc() and read chunks from
 * a file. For most C64 emulator file formats, such as T64, a small block size
 * should be enough.
 */
#define FRA_BLOCK_SIZE  (1<<16)


/** @brief  Global error code
 *
 * If this is set to T64_ERR_IO, the C library `errno` will contain further
 * information on what happened.
 */
int t64_errno;


/** @brief  Error messages
 *
 * @note    message for error code 0 has index 1, message at index 0 is for
 *          invalid error codes
 */
static const char *t64_err_msgs[] = {
    "invalid error code",
    "OK",
    "out of memory error",
    "I/O error",
    "not a T64 image",
    "index error"
};


/** @brief  Get error message for \a code
 *
 * @param   code    error code
 *
 * @return  message for \a code
 */
const char *t64_strerror(int code)
{
    if (code < T64_ERRNO_MIN || code > T64_ERRNO_MAX) {
        return t64_err_msgs[0];
    }
    return t64_err_msgs[code + 1];
}



/** @brief  Print out-of-memory message on stderr
 *
 * @param   n   number of bytes requested to malloc(3) or realloc(3)
 *
 * @return  number of characters printed on stderr
 *
 * @note    sets t64_errno to `T64_ERR_OOM`
 */
int base_err_alloc(size_t n)
{
    t64_errno = T64_ERR_OOM;
    return fprintf(stderr, "failed to allocate %lu bytes\n",
            (unsigned long)n);
}


/** @brief  Read unsigned 16-bit little endian value
 *
 * @param   p   data containing the value
 *
 * @return  unsigned 16-bit little endian value
 */
unsigned short get_uint16(const unsigned char *p)
{
    return (unsigned short)(p[0] + (1<<8) * p[1]);
}


/** @brief  Write 16-bit little endian value
 *
 * @param   p   destination of value
 * @param   v   unsigned 16-bit value
 */
void set_uint16(unsigned char *p, unsigned short v)
{
    p[0] = (unsigned char)(v & 0xff);
    p[1] = (unsigned char)((v >> 8) & 0xff);
}



/** @brief  Read unsigned 32-bit little endian value
 *
 * @param   p   data containing the value
 *
 * @return  unsigned 32-bit little endian value
 */
unsigned long get_uint32(const unsigned char *p)
{
    return (unsigned long)(get_uint16(p) + (1<<16) * p[2] + (1<<24) * p[3]);
}


/** @brief  Write 32-bit little endian value
 *
 * @param   p   destination of value
 * @param   v   unsigned 16-bit value
 */
void set_uint32(unsigned char *p, unsigned long v)
{
    set_uint16(p, v & 0xffff);
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}



/** @brief  Calculate number of 'blocks' for \a n
 *
 * @param   n   value
 */
unsigned int num_blocks(unsigned int n)
{
    return (n / 254) + (n % 254 == 0 ? 0 : 1);
}



/** @brief  Read file into memory
 *
 * This function reads data from file \a path into \a dest, allocating memory
 * while doing so. The value returned is the size of the memory allocated. If
 * this function fails -1 is returned and \*dest is set to NULL. Should an
 * empty file be read, \*dest is also set to NULL and the buffer used is freed.
 *
 * An example:
 * @code{.c}
 *
 *  unsigned char *buf;
 *  long result = fread_alloc(&buf, "citadel.d64");
 *  if (result < 0) {
 *      printf("oops!\n");
 *  } else {
 *      printf("read %ld bytes\n", result);
 *      free(buf);
 *  }
 *
 * @endcode
 *
 * @param   dest    pointer to memory to store buffer pointer
 * @param   path    path to file
 *
 * @returns size of buffer allocated, or -1 on error
 */
long fread_alloc(unsigned char **dest, const char *path)
{
    unsigned char *buffer;
    unsigned char *tmp;
    size_t bufsize = FRA_BLOCK_SIZE;
    size_t bufread = 0;
    size_t result;
    FILE *fp;

    errno = 0;
    *dest = NULL;
    fp = fopen(path, "rb");
    if (!fp) {
        t64_errno = T64_ERR_IO;
        return -1;
    }

    buffer = malloc(FRA_BLOCK_SIZE);
    if (!buffer) {
        base_err_alloc(FRA_BLOCK_SIZE);
        fclose(fp);
        return -1;
    }

    while (1) {
#ifdef BASE_DEBUG
        printf("requesting %lu bytes: ", (unsigned long)FRA_BLOCK_SIZE);
        fflush(stdout);
#endif
        result = fread(buffer + bufread, 1, FRA_BLOCK_SIZE, fp);
#ifdef BASE_DEBUG
        printf("got %lu bytes\n", (unsigned long)result);
#endif
        if (result < FRA_BLOCK_SIZE) {
            /* end of file ? */
            if (feof(fp)) {
                /* we're done */
                bufread += result;
                if (bufread == 0) {
                    /* empty file: free buffer, dest is NULL so we're safe
                     * from free()'ing dest failing */
                    free(buffer);
                } else {
                    /* realloc buffer, if it fails we still have the data: */
                    tmp = realloc(buffer, bufread);
                    if (tmp) {
                        buffer = tmp;
                    } else {
                        /* realloc failed: keep the old buffer size */
                        bufread = bufsize;
                    }
                    *dest = buffer;
#ifdef BASE_DEBUG
                    printf("reallocated to %lu bytes\n",
                            (unsigned long)bufsize);
#endif
                }
                fclose(fp);
                return (long)bufread;
            } else {
                /* I/O error */
                t64_errno = T64_ERR_IO;
                free(buffer);
                fclose(fp);
                return -1;
            }
        } else {
            /* resize buffer */
            bufsize += FRA_BLOCK_SIZE;
            bufread += FRA_BLOCK_SIZE;
#ifdef BASE_DEBUG
            printf("resizing buffer to %lu bytes\n", (unsigned long)bufsize);
#endif
            tmp = realloc(buffer, bufsize);
            if (!tmp) {
                base_err_alloc(bufsize);
                free(buffer);
                fclose(fp);
                return -1;
            }
            buffer = tmp;
        }
    }
    assert(!"should not get here!");
    return -1;
}


/** @brief  Wrapper around fwrite(3)
 *
 * @param   path    filename/path
 * @param   data    data to write to \a path
 * @param   size    number of bytes of \a data to write
 *
 * @return  bool
 */
bool fwrite_wrapper(const char *path, const unsigned char *data, size_t size)
{
    bool result = true;
    FILE *fd = fopen(path, "wb");
    if (fd == NULL) {
        t64_errno = T64_ERR_IO;
        result = false;
    } else {
        if (fwrite(data, 1, size, fd) != size) {
            result = false;
        }
        fclose(fd);
    }
    return result;
}


/** @brief  Write a prg file to the OS
 *
 * @param   path    path of file
 * @param   data    program file data, excluding start address
 * @param   size    size of program file data, excluding start address
 * @param   start   start address to use for program file
 *
 * @return  bool
 */
bool fwrite_prg(const char *path, const unsigned char *data, size_t size,
        int start)
{
    bool result = false;
    FILE *fd = fopen(path, "wb");
    if (fd == NULL) {
        t64_errno = T64_ERR_IO;
        return false;
    } else {
        /* write start address */
        if (fputc((unsigned char)(start & 0xff), fd) == EOF) {
            goto fwrite_prg_exit;
        }
        if (fputc((unsigned char)((start >> 8) & 0xff), fd) == EOF) {
            goto fwrite_prg_exit;
        }
        if (fwrite(data, 1, size, fd) != size) {
            goto fwrite_prg_exit;
        } else {
            result = true;
        }
    }
fwrite_prg_exit:
    fclose(fd);
    return result;
}
