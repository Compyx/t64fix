/** \file   base.c
 * \brief   Memory allocation, I/O, string handling, error messages
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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "base.h"

/* #define BASE_DEBUG */


/** \brief  Block size for fread_alloc()
 *
 * This size is used to allocate memory in fread_alloc() and read chunks from
 * a file. For most C64 emulator file formats, such as T64, a small block size
 * should be enough.
 */
#define FRA_BLOCK_SIZE  (1UL<<16)


/** \brief  Global error code
 *
 * If this is set to T64_ERR_IO, the C library `errno` will contain further
 * information on what happened.
 */
int t64_errno;


/** \brief  Error messages
 *
 * \note    message for error code 0 has index 1, message at index 0 is for
 *          invalid error codes
 */
static const char *t64_err_msgs[] = {
    "invalid error code",
    "OK",
    "out of memory error",
    "I/O error",
    "not a T64 image",
    "index error",
    "track number out of range",
    "sector number out of range",
    "invalid filename",
    "RLE error"
};


/** \brief  Get error message for \a code
 *
 * \param[in]   code    error code
 *
 * \return  message for \a code
 */
const char *t64_strerror(int code)
{
    if (code < T64_ERRNO_MIN || code > T64_ERRNO_MAX) {
        return t64_err_msgs[0];
    }
    return t64_err_msgs[code + 1];
}



/** \brief  Print out-of-memory message on stderr
 *
 * \param[in]   n   number of bytes requested to malloc(3) or realloc(3)
 *
 * \note    Sets t64_errno to `T64_ERR_OOM`, which is only useful for debuggers
 *          since abort() is called right after printing the message.
 */
static void base_err_alloc(size_t n)
{
    t64_errno = T64_ERR_OOM;
    fprintf(stderr, "failed to allocate %zu bytes, calling abort\n", n);
    abort();
}


/** \brief  Read unsigned 16-bit little endian value
 *
 * \param[in]   p   data containing the value
 *
 * \return  unsigned 16-bit little endian value
 */
uint16_t get_uint16(const uint8_t *p)
{
    return (uint16_t)(p[0] + (1 <<8 ) * p[1]);
}


/** \brief  Write 16-bit little endian value
 *
 * \param[out]   p   destination of value
 * \param[in]    v   unsigned 16-bit value
 */
void set_uint16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
}


/** \brief  Read unsigned 32-bit little endian value
 *
 * \param[in]   p   data containing the value
 *
 * \return  unsigned 32-bit little endian value
 */
uint32_t get_uint32(const uint8_t *p)
{
    return (uint32_t)(get_uint16(p) + (1<<16) * p[2] + (1<<24) * p[3]);
}


/** \brief  Write 32-bit little endian value
 *
 * \param[out]  p   destination of value
 * \param[in]   v   unsigned 16-bit value
 */
void set_uint32(uint8_t *p, uint32_t v)
{
    set_uint16(p, v & 0xffff);
    p[2] = (v >> 16) & 0xff;
    p[3] = (uint8_t)((v >> 24) & 0xff);
}


/** \brief  Calculate number of 'blocks' for number of bytes \a n
 *
 * Calculate how many block on a floppy \a n bytes would take.
 *
 * The result is rounded up, one byte in a block results in that entire block
 * being used, like the floppy drives do.
 *
 * \param[in]   n   number of bytes
 */
unsigned int num_blocks(unsigned int n)
{
    return (n / 254) + (n % 254 == 0 ? 0 : 1);
}


/** \brief  Read file into memory
 *
 * This function reads data from file \a path into \a dest, allocating memory
 * while doing so.
 *
 * The value returned is the size of the memory allocated. If
 * this function fails -1 is returned and \*dest is set to NULL. Should an
 * empty file be read, \*dest is also set to NULL and the buffer used is freed.
 *
 * An example:
 * \code{.c}
 *
 *  uint8_t *buf;
 *  long result = fread_alloc(&buf, "citadel.d64");
 *  if (result < 0) {
 *      printf("oops!\n");
 *  } else {
 *      printf("read %ld bytes\n", result);
 *      free(buf);
 *  }
 *
 * \endcode
 *
 * \param[out]  dest    pointer to memory to store buffer pointer
 * \param[in]   path    path to file
 *
 * \returns size of buffer allocated, or -1 on error
 */
long fread_alloc(uint8_t **dest, const char *path)
{
    uint8_t *buffer;
    uint8_t *tmp;
    size_t bufsize = FRA_BLOCK_SIZE;
    size_t bufread = 0;
    size_t result;
    FILE *fp;

    errno = 0;
    *dest = NULL;
    fp = fopen(path, "rb");
    if (fp == NULL) {
        t64_errno = T64_ERR_IO;
        return -1;
    }

    buffer = base_malloc(FRA_BLOCK_SIZE);

    while (1) {
#ifdef BASE_DEBUG
        printf("requesting %zu bytes: ", FRA_BLOCK_SIZE);
        fflush(stdout);
#endif
        result = fread(buffer + bufread, 1, FRA_BLOCK_SIZE, fp);
#ifdef BASE_DEBUG
        printf("got %zu bytes\n", result);
#endif
        if (result < FRA_BLOCK_SIZE) {
            /* end of file ? */
            if (feof(fp)) {
                /* we're done */
                bufread += result;
                if (bufread == 0) {
                    /* empty file: free buffer, dest is NULL so we're safe
                     * from free()'ing dest failing */
                    base_free(buffer);
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
                    printf("reallocated to %zu bytes\n", bufsize);
#endif
                }
                fclose(fp);
                return (long)bufread;
            } else {
                /* I/O error */
                t64_errno = T64_ERR_IO;
                base_free(buffer);
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
            buffer = base_realloc(buffer, bufsize);
        }
    }
    assert(!"should not get here!");
    return -1;
}


/** \brief  Wrapper around fwrite(3)
 *
 * \param[in]   path    filename/path
 * \param[in]   data    data to write to \a path
 * \param[in]   size    number of bytes of \a data to write
 *
 * \return  bool
 */
bool fwrite_wrapper(const char *path, const uint8_t *data, size_t size)
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


/** \brief  Write a prg file to the OS
 *
 * \param[in]   path    path of file
 * \param[in]   data    program file data, excluding start address
 * \param[in]   size    size of program file data, excluding start address
 * \param[in]   start   start address to use for program file
 *
 * \return  bool
 */
bool fwrite_prg(const char *path, const uint8_t *data, size_t size, int start)
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



/*
 * Memory allocation functions, akin to xmalloc()
 */

/** \brief  Allocate \a size bytes on the heap
 *
 * \param[in]   size    number of bytes to allocate
 *
 * \return  pointer to allocated memory
 *
 * \note    calls abort() on error to allow use of a debugger.
 */
void *base_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        base_err_alloc(size);
    }
    return ptr;
}


/** \brief  Allocate \a nmemb elements of \a size bytes and initialize to 0
 *
 * \param[in]   nmemb   number of elements to allocate
 * \param[in]   size    size of elements
 *
 * \return  pointer to allocated memory
 *
 * \note    calls abort() on error to allow use of a debugger.
 */
void *base_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        base_err_alloc(nmemb * size);
    }
    return ptr;
}


/** \brief  Free memory at \a ptr
 *
 * Wrapper around free() for symmetry with base_malloc()/base_realloc().
 *
 * \param[in]   ptr memory to free
 */
void base_free(void *ptr)
{
    free(ptr);
}


/** \brief  Reallocate memory at \a p to \a n bytes
 *
 * \param[in]   ptr     memory to reallocate
 * \param[in]   size    new size of \a ptr
 *
 * \return  pointer to reallocated memory
 *
 * \note    calls abort() on error to allow use of a debugger.
 */
void *base_realloc(void *ptr, size_t size)
{
    void *tmp = realloc(ptr, size);

    if (tmp == NULL) {
        base_err_alloc(size);
    }
    return tmp;
}


/** \brief  Test if character \a c is a path separator
 *
 * \param[in]   c   character to test
 *
 * \return  bool
 */
static bool is_path_separator(int c)
{
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}


/** \brief  Get basename component of path
 *
 * Get basename of \a path and optionally provide a pointer the extension of
 * the file, if any, when \a ext is not `NULL`.
 *
 * If no extension if found \a *ext will point to the end of \a path. 
 *
 * \param[in]   path    path to scan for basename and extension
 * \param[out]  ext     object to store pointer to extension, pass `NULL` to
 *                      ommit scanning for extension
 *
 * \return  pointer into \a path
 */
const char *base_basename(const char *path, const char **ext)
{
    const char *p;
    const char *end;

    if (path == NULL || *path == '\0') {
        return path;
    }

    /* remember the end pointer for the extension scanning later */
    p = end = path + strlen(path) - 1;
    while (p >= path && !is_path_separator(*p)) {
        p--;
    }

    if (p < path) {
        /* no path separator found */
        p = path;
    } else {
        p++;
    }

    /* do we need to scan for extension? */
    if (ext != NULL) {
        const char *x = end;

        while (x >= p) {
            if (*x == '.') {
                break;
            }
            x--;
        }
        if (x < p) {
            /* no extension found, point to the end of the string */
            *ext = end;
        } else {
            *ext = x + 1;   /* point to after the dot */
        }
    }

    return p;
}


/** \brief  Create heap-allocated copy of string \a s
 *
 * \param[in]   s   string
 *
 * \return  copy of \a s
 *
 * \note    If \a is `NULL`, an empty string will be returned, which needs to
 *          be freed after use
 */
char *base_strdup(const char *s)
{
    char *t;

    if (s == NULL || *s == '\0') {
        t = base_calloc(1, 1);
        *t = '\0';
    } else {
        size_t len = strlen(s);

        t = base_malloc(len + 1);
        memcpy(t, s, len + 1);
    }
    return t;
}


/** \brief  Create a hexdump of \a len bytes of \a src on stdout
 *
 * \param[in]   src     data to display
 * \param[in]   len     number of bytes to display
 * \param[in]   voffset virtual offset (displayed as the 'address')
 */
void base_hexdump(const uint8_t *src, size_t len, size_t voffset)
{
    uint8_t display[16];
    size_t i = 0;

    if (src == NULL || len == 0) {
        fprintf(stderr, "%s:%s:%d: error: no input\n",
                __FILE__, __func__, __LINE__);
        return;
    }

    while (i < len) {
        size_t x;
        size_t t;
        size_t c;

        printf("%05lx  ", (unsigned long)voffset);
        fflush(stdout);
        for (x = 0; x < 16 && i + x < len; x++) {
            display[x] = src[i + x];
            printf("%02x ", src[i + x]);
        }
        c = x;
        t = x;
        while (t++ < 16) {
            printf("   ");
        }
        for (t = 0; t < c; t++) {
            putchar(isprint(display[t]) ? display[t] : '.');
        }


        if (i + x >= len) {
            putchar('\n');
            return;
        }
        putchar('\n');
        i += 16;
        voffset += 16;
    }
}


/** \brief  Count number of set bits in byte \a b
 *
 * Count bits the Brian Kernighan way.
 *
 * \param[in]   b   byte to process
 *
 * \return  number of set bits in \a b
 */
int popcount_byte(uint8_t b)
{
    int c = 0;

    while (b) {
        b &= (uint8_t)(b - 1U);
        c++;
    }
    return c;
}

