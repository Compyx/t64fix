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

/** \file   t64.c
 * \brief   t64 image fixing
 *
 * So far, all functions that fix stuff in the t64 files do so by writing into
 * t64_record_t and t64_image_t instances, so the actual t64 file isn't touched
 * until it is written to the OS, either using the original file or a copy.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "base.h"
#include "cbmdos.h"
#include "petasc.h"

#include "t64.h"


/* {{{ Static data */

/** \brief  C1541 file types
 */
static const char *c1541_types[] = {
    "frz", "seq", "prg", "rel", "usr", "???", "???", "???"
};


/** \brief  The correct 'magic' for t64 files
 *
 * The string is in ASCII, padded to 32 bytes with 0x00. This is based on a
 * file generated by C64S 2.52.
 */
static const char c64s_magic[T64_HDR_MAGIC_LEN] = "C64S tape image file";


/** \brief  List of 'magic bytes' found in different t64 files
 */
static const char *magic_strings[] = {
    c64s_magic,         /* the correct one */
    "C64S tape file",
    "C64 tape image file"
};


/** \brief  Status strings for records
 */
static const char *status_strings[] = {  "OK", "fixed", "skipped" };


/* }}} */


/* {{{ T64 header functions */

/** \brief  Check magic bytes in header of \a image
 *
 * This functions checks the magic bytes in the header against known magic
 * bytes found in various tape images. The first entry checked is the correct
 * one, so if this function returns 0, we have the correct magic, if this
 * function returns > 0, we have some custom magic. If it returns < 0, no
 * magic was found.
 *
 * \param[in]   image   t64 image
 *
 * \return  index in `magic_strings` or -1 when not found
 */
static int t64_check_magic(const t64_image_t *image)
{
    size_t i;

    for (i = 0; i < sizeof magic_strings / sizeof magic_strings[0]; i++) {
        if (memcmp(magic_strings[i], image->data + T64_HDR_MAGIC, 
                    strlen(magic_strings[i])) == 0) {
            return (int)i;
        }
    }
    return -1;
}


/** \brief  Parse header to determine if it is a t64 image, apply some fixes
 *
 * This function checks the header's magic against known magic strings. If no
 * magic was found, 0 (false) is returned, otherwise, some header fixes are
 * applied and 1 (true) is returned
 *
 * \return  boolean
 */
static bool t64_parse_header(t64_image_t *image, int quiet)
{
    int result = t64_check_magic(image);
    if (result < 0) {
        if (!quiet) {
            printf("t64fix: fatal: couldn't find magic bytes, aborting\n");
        }
        return false;
    } else {
        if (result > 0) {
            if (!quiet) {
                printf("t64fix: warning: fixing header magic bytes\n");
            }
            image->fixes++;
        }
        strcpy((char *)(image->magic), magic_strings[result]);
    }
    /* get file record max and used counters */
    image->rec_max = get_uint16(image->data + T64_HDR_REC_MAX);
    image->rec_used = get_uint16(image->data + T64_HDR_REC_USED);
    if (image->rec_max == 0) {
        if (!quiet) {
            printf("t64fix: warning: maximum records count reported as "
                "0, adjusting to 1\n");
        }
        image->rec_max = 1;
        image->fixes++;
    }
    if (image->rec_used == 0) {
        if (!quiet) {
            printf("t64fix: warning: current records count reported as "
                "0, adjusting to 1\n");
        }
        /* this fix is required for the other fixes to work */
        image->rec_used = 1;
        image->fixes++;
    }

    if (image->rec_used > image->rec_max) {
        if (!quiet) {
            printf("t64fix: warning: header reports more used records than "
                    "available records (%d > %d), fixing\n",
                    (int)(image->rec_used), (int)(image->rec_max));
        }
        image->rec_used = image->rec_max;
        image->fixes++;
    }

    /* copy tape name: warning: not 0-terminated */
    memcpy(image->tapename, image->data + T64_HDR_NAME, T64_HDR_NAME_LEN);

    /* get tape version */
    image->version = get_uint16(image->data + T64_HDR_VERSION);
    return true;
}


/** \brief  Write correct header data in \a image
 *
 * \param[in]   image   t64 image
 */
static void t64_write_header(t64_image_t *image)
{
    /* write the correct magic */
    memcpy(image->data + T64_HDR_MAGIC, c64s_magic, T64_HDR_MAGIC_LEN);

    /* write tape name */
    memcpy(image->data + T64_HDR_NAME, image->tapename, T64_HDR_NAME_LEN);

    /* write version */
    set_uint16(image->data + T64_HDR_VERSION, 0x101);

    /* write number of available file records and number of used records */
    set_uint16(image->data + T64_HDR_REC_MAX, image->rec_max);
    set_uint16(image->data + T64_HDR_REC_USED, image->rec_used);
}

/* }}} */


/* {{{ T64 file record handling */

/** \brief  Comparison function for qsort() call to sort records on data offset
  *
  * \param[in]  p1  pointer to record 1
  * \param[in]  p2  pointer to record 2
  *
  * \return -1 if r1<r2, 0 if r1==r2, 1 if r1>r2
  */
static int compar_offset(const void *p1, const void *p2)
{
    const t64_record_t *r1 = p1;
    const t64_record_t *r2 = p2;

    if (r1->offset < r2->offset) {
        return -1;
    } else if (r1->offset > r2->offset) {
        return 1;
    } else {
        return 0;
    }
}


/** \brief  Comparison function for qsort() call to sort records on index
  *
  * \param[in]  p1  pointer to record 1
  * \param[in]  p2  pointer to record 2
  *
  * \return -1 if r1<r2, 0 if r1==r2, 1 if r1>r2
  */
static int compar_index(const void *p1, const void *p2)
{
    const t64_record_t *r1 = p1;
    const t64_record_t *r2 = p2;

    if (r1->index < r2->index) {
        return -1;
    } else if (r1->index > r2->index) {
        return 1;
    } else {
        return 0;
    }
}


/** \brief  Read a single record into \a record using \a data
 *
 * \param[in]   record  t64 file record
 * \param[in]   data   index in file records
 *
 * \return  new file record or NULL on malloc(3) failure
 */
static void t64_read_record(t64_record_t *record, const unsigned char *data)
{
    memcpy(record->filename, data + T64_REC_FILENAME, T64_REC_FILENAME_LEN);
    record->offset = get_uint32(data + T64_REC_CONTENTS);
    record->start_addr = get_uint16(data + T64_REC_START_ADDR);
    record->end_addr = get_uint16(data + T64_REC_END_ADDR);
    record->c64s_ftype = data[T64_REC_C64S_FILETYPE];
    record->c1541_ftype = data[T64_REC_C1541_FILETYPE];
    record->index = 0;
    record->status = T64_REC_OK;
}


/** \brief  Write corrected file record into image data
 *
 * \param[in]   record  t64 file record
 * \param[out]  dest    destination in image data
 */
static void t64_write_record(t64_record_t *record, unsigned char *dest)
{
    dest[T64_REC_C64S_FILETYPE] = record->c64s_ftype;
    dest[T64_REC_C1541_FILETYPE] = record->c1541_ftype;
    set_uint16(dest + T64_REC_START_ADDR, record->start_addr);
    set_uint16(dest + T64_REC_END_ADDR, record->real_end_addr);
    set_uint32(dest + T64_REC_CONTENTS, record->offset);
    memcpy(dest + T64_REC_FILENAME, record->filename, T64_REC_FILENAME_LEN);
}


/** \brief  Print file record in \a image at \a index on stdout
 *
 * \param[in]   image   t64 image
 * \param[in]   index   index in image
 */
static void t64_print_record(const t64_image_t *image, int index)
{
    t64_record_t *record = image->records + index;
    int size = record->end_addr - record->start_addr;
    char filename[T64_REC_FILENAME_LEN + 1];

    memcpy(filename, record->filename, T64_REC_FILENAME_LEN);
    filename[T64_REC_FILENAME_LEN] = '\0';

    printf("%5d  \"%-16s\" %s  $%04x-$%04x  $%04x-$%04x  %s\n",
            num_blocks((unsigned int)size),
            filename,
            c1541_types[record->c1541_ftype & 0x07],
            record->start_addr, record->end_addr,
            record->start_addr, record->real_end_addr,
            status_strings[record->status]);
}
/* }}} */


/* {{{ T64 image file handling */

/** \brief  Allocate new, empty, t64 image
 *
 * @internal
 *
 * \return  new t64 image or NULL on failure
 */
static t64_image_t *t64_new(void)
{
    t64_image_t *image = base_malloc(sizeof *image);
    image->path = NULL;
    image->data = NULL;
    image->size = 0;
    image->records = NULL;
    image->rec_max = 0;
    image->rec_used = 0;
    image->fixes = 0;
    return image;
}


/** \brief  Open t64 container
 *
 * \param[in]   path    path to container
 * \param[in]   quiet   don't output anything on stdout/stderr
 *
 * \return  image or NULL on failure
 */
t64_image_t *t64_open(const char *path, int quiet)
{
    t64_image_t *image;
    long size;
    int i;

    image = t64_new();
    if (image == NULL) {
        return NULL;
    }
    image->path = path;
    size = fread_alloc(&(image->data), path);
    if (size < 0) {
        /* error already reported by fread_alloc() */
        t64_free(image);
        return NULL;
    }
    /* store image size */
    image->size = (size_t)size;
    /* parse header for required information */
    if (!t64_parse_header(image, quiet)) {
        /* header parsing failed, bail: */
        t64_free(image);
        return NULL;
    }

    /* allocate and read records */
    image->records = base_malloc(sizeof *(image->records) * image->rec_used);
    for (i = 0; i < image->rec_used; i++) {
        t64_read_record(image->records + i,
                image->data + T64_RECORDS_OFFSET + i * T64_RECORD_SIZE);
        (image->records + i)->index= i;
    }

    return image;
}


/** \brief  Free memory used by t64 image
 *
 * \param[in]   image   t64 image
 */
void t64_free(t64_image_t *image)
{
    if (image->data != NULL) {
        base_free(image->data);
    }
    if (image->records != NULL) {
        base_free(image->records);
    }
    base_free(image);
}


/** \brief  Verify data in \a image, optionally fixing it
 *
 * \param[in]   image   t64 image
 * \param[in]   quiet   don't output anything on stdout/stderr
 *
 * \return  number of fixes, or -1 on error
 */
int t64_verify(t64_image_t *image, int quiet)
{
    size_t rec_size;    /* file size according to record */
    size_t act_size;    /* actual file size */
    int i;

    /* check maximum record count */
    if (image->rec_max == 0) {
        if (!quiet) {
            printf("t64fix: warning: maximum records count reported as "
               "0, adjusting to 1\n");
        }
        image->rec_max = 1;
        image->fixes++;
    }
    /* check number of used records */
    if (image->rec_used == 0) {
        if (!quiet) {
            printf("t64fix: warning: used records count reported as 0, "
                    "adjusting to 1\n");
        }
        /* this fix is required for the other fixes to work */
        image->rec_used = 1;
        image->fixes++;
    }

    /* Fix end addresses by sorting file records on data offset and then using
     * either the data offset of the next entry, or the length of the t64 file
     * to get the proper end address */

    /* sort entries based on offset */
    qsort(image->records, (size_t)(image->rec_used),
            sizeof *(image->records), compar_offset);

    /* process records, reporting any invalid data, optionally fixing it */
    for (i = 0; i < image->rec_used; i++) {
        t64_record_t *record = image->records + i;
        t64_record_t *next;

        /* check file type */
        if (record->c64s_ftype > 0x01) {
            /* memory snapshot */
            record->status = T64_REC_SKIPPED;
        } else {
            if (record->c1541_ftype < 0x80 || record->c1541_ftype >= 0x85) {
                if (!quiet) {
                    printf("t64fix: illegal value $%02x for C1541 file type, "
                            "assuming $82 (prg)\n", record->c1541_ftype);
                }
                record->c1541_ftype = 0x82;
                record->status = T64_REC_FIXED;
                image->fixes++;
            }

            record = image->records + i;
            if (i < image->rec_used - 1) {
                next = image->records + i + 1;
            } else {
                next = NULL;
            }


            /* get reported size */
            rec_size = (size_t)(record->end_addr - record->start_addr);
            /* determine actual size */
            if (i < image->rec_used - 1) {
                act_size = (size_t)(next->offset - record->offset);
            } else {
                act_size = (size_t)(image->size - record->offset);
            }
            if (rec_size != act_size) {
                if (i == image->rec_used -1 && rec_size < act_size) {
                    /* don't fix last record when actual size is larger: some
                     * T64's have padding for the last record */
                    continue;
                }
                if (!quiet) {
                    printf("t64fix: %d: reported size of $%04lx does not "
                            "match actual size of $%04lx\n",
                            i, (unsigned long)rec_size, (unsigned long)act_size);
                }
                record->status = T64_REC_FIXED;
                image->fixes++;
                record->real_end_addr = (unsigned short)(record->start_addr +
                        act_size);
            } else {
                record->real_end_addr = record->end_addr;
            }
        }

    }
    /* restore original record order */
    qsort(image->records, (size_t)(image->rec_used),
            sizeof *(image->records), compar_index);

    return image->fixes;
}


/** \brief  Print a 79 chars wide separator on stdout
 */
static void print_sep(void)
{
    puts("-------------------------------------------------------------------"
         "------------");
}


/** \brief  Dump t64 image information and file records on stdout
 *
 * \param[in]   image   t64 image
 */
void t64_dump(const t64_image_t *image)
{
    char tapename[T64_HDR_NAME_LEN + 1];
    char magic[T64_HDR_MAGIC_LEN + 1];
    int i;

    /* copy tapename, TODO: translate PETSCII to ASCII */
    memcpy(tapename, image->tapename, T64_HDR_NAME_LEN);
    tapename[T64_HDR_NAME_LEN] = '\0';  /* terminated name */
    /* remove padding */
    i = T64_HDR_NAME_LEN - 1;
    while (i >= 0 && tapename[i] == 0x20) {
        tapename[i--] = '\0';
    }


    /* copy magic */
    memcpy(magic, image->magic, T64_HDR_MAGIC_LEN);
    magic[T64_HDR_MAGIC_LEN] = '\0';    /* terminate magic */
    /* remove padding */
    i = T64_HDR_MAGIC_LEN - 1;
    while (i >= 0 && magic[i] == 0x20) {
        magic[i--] = '\0';
    }

    print_sep();
    printf("tape magic  : \"%s\"\n", magic);
    printf("tape version: %04x\n", image->version);
    printf("tape name   : \"%s\"\n", tapename);
    printf("file records: %d/%d\n", (int)(image->rec_used),
            (int)(image->rec_max));
    print_sep();

    /* print file records */
    printf("blocks filename           type rep. memory  real memory  status\n");
    for (i = 0; i < image->rec_used; i++) {
        t64_print_record(image, i);
    }
    print_sep();
    if (image->fixes > 0) {
        printf("faulty image: fixes applied: %d\n", image->fixes);
    } else {
        puts("OK, proper image");
    }
}


/** \brief  Write t64 image to OS
 *
 * Write corrected image to host filesystem.
 *
 * This function stores corrected header and directory data in \a image before
 * writing to host.
 *
 * \param[in]   image   t64 image
 * \param[in]   path    path/filename of image
 *
 * \return  boolean
 */
bool t64_write(t64_image_t *image, const char *path)
{
    /* write corrected header in image data */
    t64_write_header(image);

    for (int i = 0; i < image->rec_used; i++) {
        t64_write_record(image->records + i,
                image->data + T64_RECORDS_OFFSET + (i * T64_RECORD_SIZE));
    }

    if (!fwrite_wrapper(path, image->data, image->size)) {
        return false;
    }

    return true;
}


/** \brief  Create T64 image and add files
 *
 * \param[in]   path    name of T64 image to create
 * \param[in]   args    list of files to add
 * \param[in]   nargs   number of elements in \a args
 * \param[in]   quiet   don't output anything on stdout
 *
 * \return  new T64 image instance or `NULL` on error
 */
t64_image_t *t64_create(const char *path, const char **args, int nargs, bool quiet)
{
    t64_image_t *image;
    uint32_t dir_size;
    uint32_t data_offset;
    const char *img_name;
    const char *img_ext;
    size_t img_name_len;
    int n;

    if (!quiet) {
        printf("Creating new t64 image '%s':\n", path);
    }
    image = t64_new();
    if (image == NULL) {
        return NULL;    /* error already reported */
    }

    /* calculate directory size */
    dir_size = (unsigned int)nargs * T64_RECORD_SIZE;
    if (!quiet) {
        printf(".. directory size = $%04x\n", (unsigned int)dir_size);
    }
    /* calculate file data offset */
    data_offset = T64_RECORDS_OFFSET + dir_size;
    if (!quiet) {
        printf(".. data offset = $%04x\n", (unsigned int)data_offset);
    }

    /* allocate data for records */
    image->records = base_malloc(sizeof *(image->records) * (size_t)nargs);

    /* allocate data for header and directory and initialize header */
    image->data = base_malloc(data_offset);
    image->size = data_offset;
    memset(image->data, 0, data_offset);

    /* add files to image */
    for (n = 0; n < nargs; n++) {

        t64_record_t record;
        uint8_t *data;
        uint8_t petname[CBMDOS_FILENAME_MAX];
        const char *ascname;
        const char *ext;
        long len;

        len = fread_alloc(&data, args[n]);
        if (!quiet) {
            printf(".. file '%s' is %ld ($%04lx) bytes.\n",
                   args[n], len, (unsigned long)len);
        }
        if (len < 0) {
            fprintf(stderr, "t64fix: failed to read file '%s'.\n", args[n]);
            base_free(data);
            t64_free(image);
            return NULL;
        }

        /* create directory record from file data */
        memset(&record, 0, sizeof(record));

        /* set PETSCII filename using the file's ASCII basename */
        ascname = base_basename(args[n], &ext);
#if 0
        printf(".. basename = '%s', ext = '%s'\n", ascname, ext);
#endif
        asc_to_pet_str(petname, ascname, CBMDOS_FILENAME_MAX);
        memcpy(record.filename, petname, CBMDOS_FILENAME_MAX);

        /* set start, end and real_end */
        record.start_addr = get_uint16(data);
        record.end_addr = (uint16_t)(len - 2 + record.start_addr);
        record.real_end_addr = record.end_addr;
        if (!quiet) {
            printf(".... start address = $%04x\n", record.start_addr);
            printf(".... end address   = $%04x\n", record.end_addr);
        }

        /* set C64S file type */
        record.c64s_ftype = 0x01;   /* normal file */
        /* set CBMDOS file type and flags */
        record.c1541_ftype = CBMDOS_FILETYPE_PRG | CBMDOS_CLOSED_MASK;

        /* realloc data after header + dir */
        image->data = base_realloc(image->data, image->size + (size_t)len - 2);
        /* copy file data stripped of load address */
        memcpy(image->data + image->size, data + 2, (size_t)(len - 2));
        /* store data size */
        record.offset = (uint32_t)image->size;
        image->size += (size_t)len - 2;
        /* free file data */
        base_free(data);

        /* store directory entry in t64 image instance, not its raw data */
        memcpy(image->records + n, &record, sizeof(record));

        if (!quiet) {
            printf(".... added '%s'\n", ascname);
        }
    }

    /* set directory size and entry count */
    image->rec_used = (uint16_t)n;
    image->rec_max = (uint16_t)n ;

    /* set internal image name using the basename of the t64 file without
     * extension:
     */
    img_name = base_basename(path, &img_ext);

    /* T64's have their tape name padded with spaces, so clear the name by
     * writing spaces:
     */
    memset(image->tapename, 0x20, T64_HDR_NAME_LEN);
#if 0
    printf("basename: '%s', extension: '%s'\n", img_name, img_ext);
#endif
    if (*img_ext == '\0') {
        /* no extension */
        img_name_len = strlen(img_name);
    } else {
        img_name_len = strlen(img_name) - strlen(img_ext) - 1;
    }
#if 0
    printf("name len without ext = %zu\n", img_name_len);
#endif
    if (img_name_len > 0) {
        /* copy at most 24 chars */
        if (img_name_len > T64_HDR_NAME_LEN) {
            img_name_len = T64_HDR_NAME_LEN;
        }
        memcpy(image->tapename, img_name, img_name_len);
        if (!quiet) {
            char tapename[T64_HDR_NAME_LEN + 1];

            memset(tapename, 0, sizeof(tapename));
            memcpy(tapename, img_name, img_name_len);
            printf(".. setting tape name to '%s'.\n", tapename);
        }
    }

    if (!quiet) {
        printf(".. created new image with %d entries, "
               "%zu ($%zx) bytes.\n",
               n, image->size, image->size);
    }

    return image;
}

/* }}} */
