/** \file   d64.c
 * \brief   D64 handling
 */

/*
 * This file is part of zipcode-conv
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "base.h"
#include "cbmdos.h"
#include "petasc.h"

#include "d64.h"


/** \brief  DOS type strings
 */
static const char *dos_types[] = {
    "CBM DOS",
    "SpeedDOS",
    "DolphinDOS",
    "Professional DOS",
    "Prologic DOS"
};


/** \brief  Speed zones table for D64 images
 */
static const d64_speedzone_t speedzones[] = {
    {  1, 17, 21 },
    { 18, 24, 19 },
    { 25, 30, 18 },
    { 31, 40, 17 }
};




/** \brief  Get offset in bytes for block at (\a track, \a sector)
 *
 * \param[in]   track   track number
 * \param[in]   sector  sector number
 *
 * \return  offset in bytes or -1 on failure
 * \throw   T64_ERR_D64_TRACK_RANGE
 * \throw   T64_ERR_D64_SECTOR_RANGE
 */
long d64_block_offset(int track, int sector)
{
    int zone = 0;
    long offset = 0;
    int tracks;

    /* preliminary checks on track and sector number */
    if (track < D64_TRACK_MIN || track > D64_TRACK_MAX_EXT) {
        t64_errno = T64_ERR_D64_TRACK_RANGE;
        return -1;
    }
    if (sector < D64_SECTOR_MIN || sector > D64_SECTOR_MAX) {
        t64_errno = T64_ERR_D64_SECTOR_RANGE;
        return -1;
    }

    while (zone < (int)(sizeof speedzones / sizeof speedzones[0])
            && (track > speedzones[zone].track_max)) {
        /* add complete zone */
        tracks = speedzones[zone].track_max - speedzones[zone].track_min + 1;
        offset += tracks * D64_BLOCK_SIZE_RAW * speedzones[zone].sectors;
        zone++;
    }

    /* final sector number check */
    if (sector >= speedzones[zone].sectors) {
        return -1;
    }

    tracks = track - speedzones[zone].track_min;
    offset += tracks * D64_BLOCK_SIZE_RAW * speedzones[zone].sectors;
    return offset + sector * D64_BLOCK_SIZE_RAW;
}


/** \brief  Get offset in bytes for \a track
 *
 * \param[in]   track   track number
 *
 * \return  offset in bytes or -1 on failure
 * \throw   T64_ERR_D64_TRACK_RANGE
 */
long d64_track_offset(int track)
{
    return d64_block_offset(track, 0);
}


/** \brief  Get maximum sector number of \a track
 *
 * Get the maximum allow sector number for track.
 *
 * \param[in]   track   track number
 *
 * \return  maximum sector number (indexed by 0) for \a track or -1 on error
 * \throw   T64_ERR_D64_TRACK_RANGE
 *
 * \note    Assumes a 40-track image, so getting a proper error for 35-track
 *          images requires an external check for the track count
 */
int d64_track_max_sector(int track)
{
    int zone = 0;

    if (track < D64_TRACK_MIN) {
        t64_errno = T64_ERR_D64_TRACK_RANGE;
        return -1;
    }

    while (zone < (int)(sizeof speedzones / sizeof speedzones[0])) {
        if (track <= speedzones[zone].track_max) {
            /* found zone */
            return speedzones[zone].sectors;
        }
        zone++;
    }
    t64_errno = T64_ERR_D64_TRACK_RANGE;
    return -1;
}




/** \brief  Check if \a track number is valid for \a d64
 *
 * \param[in]   d64     D64 handle
 * \param[in]   track   track number
 *
 * \return  true if \a track is valid for image \a d64
 * \throw   T64_ERR_D64_NULL
 * \throw   T64_ERR_D64_TRACK_RANGE
 */
bool d64_track_is_valid(const d64_t *d64, int track)
{
    int track_max;

    track_max = (d64->type == D64_TYPE_CBMDOS ? 35 : 40);

    if (track < D64_TRACK_MIN || track > track_max) {
        t64_errno = T64_ERR_D64_TRACK_RANGE;
        return false;
    }
    return true;
}


/** \brief  Determine if (\a track, \a sector) is a valid block for \a d64
 *
 * \param[in]   d64     D64 image
 * \param[in]   track   track number
 * \param[in]   sector  sector number
 *
 * \return  boolean
 * \throw   T64_ERR_D64_TRACK_RANGE
 * \throw   T64_ERR_D64_SECTOR_RANGE
 */
bool d64_block_is_valid(const d64_t *d64, int track, int sector)
{
    int sector_max;

    if (!d64_track_is_valid(d64, track)) {
        return false;   /* error codes already set */
    }

    sector_max = d64_track_max_sector(track);
    if (sector < D64_SECTOR_MIN || sector > sector_max) {
        t64_errno = T64_ERR_D64_SECTOR_RANGE;
        return false;
    }
    return true;
}


/** \brief  Read block (\a track,\a sector) in \a d64 into \a buffer
 *
 * \param[in]   d64     D64 handle
 * \param[out]  buffer  buffer to store block data
 * \param[in]   track   track number of block
 * \param[in]   sector  sector number of sector
 *
 * \return  true on success
 * \throw   T64_ERR_D64_TRACK_RANGE
 * \throw   T64_ERR_D64_SECTOR_RANGE
 */
bool d64_block_read(const d64_t *d64,
                        uint8_t *buffer,
                        int track, int sector)
{
    long offset;

    if (!d64_track_is_valid(d64, track)) {
        return false;
    }

    offset = d64_block_offset(track, sector);
    if (offset < 0) {
        return false;
    }

    memcpy(buffer, d64->data + offset, D64_BLOCK_SIZE_RAW);
    return true;
}


/** \brief  Write \a buffer into \a d64 at block (\a track,\a sector)
 *
 * \param[in]   d64     D64 handle
 * \param[out]  buffer  buffer to read data from
 * \param[in]   track   track number of block
 * \param[in]   sector  sector number of sector
 *
 * \return  true on success
 * \throw   T64_ERR_D64_TRACK_RANGE
 * \throw   T64_ERR_D64_SECTOR_RANGE
 */
bool d64_block_write(d64_t *d64,
                         const uint8_t *buffer,
                         int track, int sector)
{
    long offset;

    if (!d64_track_is_valid(d64, track)) {
        return false;
    }

    offset = d64_block_offset(track, sector);
    if (offset < 0) {
        return false;
    }

    memcpy(d64->data + offset, buffer, D64_BLOCK_SIZE_RAW);
    return true;
}


/** \brief  Initialize \a d64 for use
 *
 * \param[out]  d64 D64 handle
 */
void d64_init(d64_t *d64)
{
    d64->path = NULL;
    d64->data = NULL;
    d64->size = 0;
    d64->type = D64_TYPE_CBMDOS;
}


/** \brief  Allocate memory in \a d64 for a D64 of \a type
 *
 * \param[in,out]   d64     D64 handle
 * \param[in]       type    D64 type
 */
void d64_alloc(d64_t *d64, d64_type_t type)
{
    size_t size;

    if (type ==  D64_TYPE_CBMDOS) {
        size = D64_SIZE_CBMDOS;
    } else {
        size = D64_SIZE_EXTENDED;
    }

    d64->data = calloc(size, 1LU);
    d64->size = size;
}


/** \brief  Free memory used by member of \a d64
 *
 * \param[in,out]   d64 D64 handle
 */
void d64_free(d64_t *d64)
{
    if (d64->path != NULL) {
        free(d64->path);
    }
    if (d64->data != NULL) {
        free(d64->data);
    }
}


/** \brief  Read D64 file
 *
 * \param[in,out]   d64     D64 handle
 * \param[in]       path    path to D64 image file
 * \param[in]       type    D64 type (ignored when 35 tracks)
 *
 * \return  TRUE if succesfull
 */
bool d64_read(d64_t *d64, const char *path, d64_type_t type)
{
    long result;

    /* Attempt to load image data */
    result = fread_alloc(&(d64->data), path);
    base_debug("got %ld bytes\n", result);
    if (result != D64_SIZE_CBMDOS && result != D64_SIZE_EXTENDED) {
        /* Failed */
        base_debug("error: invalid image size\n");
        free(d64->data);
        d64->data = NULL;
        return false;
    }

    /* OK */
    d64->path = base_strdup(path);
    d64->size = (size_t)result;
    if (result == D64_SIZE_CBMDOS) {
        d64->type = D64_TYPE_CBMDOS;
    } else {
        d64->type = type;
    }
    return true;
}


/** \brief  Dump some generic info about \a d64 on stdout
 *
 * \param[in]   d64     D64 handle
 */
void d64_dump_info(const d64_t *d64)
{
    printf("type: %s\n", dos_types[d64->type]);
    printf("path: %s\n", d64->path != NULL ? d64->path : "<unset>");
    printf("size: $%lx\n", (unsigned long)d64->size);
}


/** \brief  Show a hexdump of the BAM of \a d64
 *
 * \param[in]   d64 D64 handle
 */
void d64_dump_bam(const d64_t *d64)
{
    base_hexdump(d64->data + D64_BAM_OFFSET,
                 D64_BLOCK_SIZE_RAW,
                 D64_BAM_OFFSET);
}


/** \brief  Write \a d64 to host file system
 *
 * Write the image in \a d64 to the host file system. If \a path is `NULL`, use
 * the path in \a d64. If that is also `NULL`, fail. Using a non-NULL \a path
 * will replace the old path in \a d64.
 *
 * \param[in,out]   d64     D64 handle
 * \param[in]       path    path to write to (NULL to use \a d64's path
 *
 * \return  boolean
 * \throw   T64_ERR_D64_INVALID_FILENAME
 */
bool d64_write(d64_t *d64, const char *path)
{
    if (path == NULL && d64->path == NULL) {
        t64_errno = T64_ERR_D64_INVALID_FILENAME;
        return false;
    }

    /* use new path? */
    if (path != NULL) {
        if (d64->path != NULL) {
            free(d64->path);
            d64->path = base_strdup(path);
        }
    }

    if (fwrite_wrapper(d64->path, d64->data, d64->size)) {
        return false;
    }
    return true;
}


/** \brief  Read BAM entry for \a track in \a d64 into \a bament
 *
 * Only works for 35-track images at the moment.
 *
 * \param[in]   d64     D64 image
 * \param[out]  bament  storage for raw BAM entry (4 bytes)
 * \param[in]   track   track number
 *
 * \return  TRUE on success
 */
bool d64_bament_read(const d64_t *d64, uint8_t *bament, int track)
{
    uint8_t *bam;
    long offset;

    if (track < D64_TRACK_MIN || track > D64_TRACK_MAX) {
        return false;
    }

    offset = d64_block_offset(D64_BAM_TRACK, D64_BAM_SECTOR);
    bam = d64->data + offset;

    memcpy(bament,
           bam + D64_BAM_TRACKS + (track - 1) * D64_BAMENT_SIZE,
           D64_BAMENT_SIZE);

    return true;
}


/** \brief  Determine number of blocks free on \a d64
 *
 * \param[in]   d64     D64 image
 *
 * \return  blocks free
 *
 * \todo    Mask out unused bits for invalid sectors
 */
int d64_blocks_free(d64_t *d64)
{
    int blocks = 0;

    for (int track = D64_TRACK_MIN; track <= D64_TRACK_MAX; track++) {
        /* exclude track 18 */
        if (track != D64_DIR_TRACK) {
            uint8_t bament[D64_BAMENT_SIZE];
            int popcount;

            d64_bament_read(d64, bament, track);

            popcount = popcount_byte(bament[D64_BAMENT_BITMAP + 0])
                     + popcount_byte(bament[D64_BAMENT_BITMAP + 1])
                     + popcount_byte(bament[D64_BAMENT_BITMAP + 2]);
#if 0
            base_debug("popcount for track %d = %d\n", track, popcount);
#endif
            blocks += popcount;
        }
    }
    return blocks;
}


/** \brief  Initialize d64 dirent
 *
 * \param[out]  dirent  D64 director entry object
 * \param[in]   d64     D64 image
 */
void d64_dirent_init(d64_dirent_t *dirent, d64_t *d64)
{
    if (d64 == NULL) {
        fprintf(stderr,
                "%s:%d: got NULL for D64!\n",
                __func__, __LINE__);
        exit(1);
    }
    dirent->d64 = d64;
    memset(dirent->name, 0, D64_DISKNAME_MAXLEN);
    memset(dirent->geos, 0, D64_DIRENT_GEOS_SIZE);
    dirent->filetype = 0;
    dirent->size = 0;
    dirent->blocks = 0;
    dirent->track = 0;
    dirent->sector = 0;
    dirent->dir_track = 0;
    dirent->dir_sector = 0;
    dirent->rel_length = 0;
    dirent->ssb_track = 0;
    dirent->ssb_sector = 0;
}


/** \brief  Read data into \a dirent from \a data
 *
 * \param[out]  dirent  D64 directory entry
 * \param[in]   data    raw directory entry data
 */
void d64_dirent_read(d64_dirent_t *dirent, const uint8_t *data)
{
    long size;

    /* $00 (useless) */
    dirent->dir_track = data[D64_DIRENT_DIR_TRACK];
    /* $01 */
    dirent->dir_sector = data[D64_DIRENT_DIR_SECTOR];
    /* $02 */
    dirent->filetype = data[D64_DIRENT_FILETYPE];
    /* $03 */
    dirent->track = data[D64_DIRENT_TRACK];
    /* $04 */
    dirent->sector = data[D64_DIRENT_SECTOR];
    /* $05-$14 */
    memcpy(dirent->name, data + D64_DIRENT_FILENAME, CBMDOS_FILENAME_MAX);
    /* $15 */
    dirent->ssb_track = data[D64_DIRENT_SSB_TRACK];
    /* $16 */
    dirent->ssb_sector = data[D64_DIRENT_SSB_SECTOR];
    /* $17 */
    dirent->rel_length = data[D64_DIRENT_REL_LENGTH];
    /* $18-$1d */
    memcpy(dirent->geos, data + D64_DIRENT_GEOS, D64_DIRENT_GEOS_SIZE);
    /* $1e-$1f */
    dirent->blocks = (uint8_t)(data[D64_DIRENT_BLOCKS_LSB]
            + 256 * data[D64_DIRENT_BLOCKS_MSB]);

    /* get file size in bytes */
    if (d64_block_is_valid(dirent->d64, dirent->track, dirent->sector)) {
        base_debug("getting file size in bytes for (%d,%d):",
                dirent->track, dirent->sector);
        size = d64_file_size(dirent->d64, dirent->track, dirent->sector);
        base_debug("file size = %ld", size);
        if (size >= 0) {
            dirent->size = (size_t)size;
        }
    }
}


/** \brief  Initialize D64 directory entry iterator
 *
 * \param[in,out]   iter    D64 directory entry iterator
 * \param[in]       d64     D64 image
 *
 * \return  true if at least on dirent was found, false otherwise
 */
bool d64_dirent_iter_init(d64_dirent_iter_t *iter, d64_t *d64)
{
    uint8_t buffer[D64_BLOCK_SIZE_RAW];
    iter->d64 = d64;
    iter->sector = D64_DIR_SECTOR;
    iter->offset = 0;
    iter->index = 0;

    if (d64 == NULL) {
        exit(1);
    }

    /* read raw initial block at (18,1) */
    if (!d64_block_read(d64, buffer, D64_DIR_TRACK, D64_DIR_SECTOR)) {
        perror(__func__);
        exit(1);
    }
    /* convert to dirent */
    iter->dirent.d64 = d64;    /* !! */
    d64_dirent_read(&(iter->dirent), buffer);

    return (bool)(iter->dirent.name[0]);
}


/** \brief  Move D64 directory entry iterator to the next entry
 *
 * \param[in,out]   iter    D64 directory entry iterator
 *
 * \return  TRUE if a next entry was found, FALSE when end-of-dir
 */
bool d64_dirent_iter_next(d64_dirent_iter_t *iter)
{
    uint8_t buffer[D64_BLOCK_SIZE_RAW];
    if (iter->index == D64_DIRENT_MAX - 1
            || iter->dirent.name[0] == 0) {
        return false;
    }

    /* move to next entry */
    if (iter->offset < 0xe0) {
        /* move inside sector */
        iter->offset += D64_DIRENT_SIZE;
    } else {
        /* check for next dir sector */
        long offset;
        uint8_t *data;
        int next_sector;


        base_debug("Checking for next dir sector\n");
        offset = d64_block_offset(D64_DIR_TRACK, iter->sector);
        if (offset < 0) {
            return false;
        }
        data = iter->d64->data + offset;
        next_sector = data[1];
        base_debug("Next block = (18,%d)\n", next_sector);
        if (next_sector == 255) {
            return false;
        }
        iter->offset = 0;
        iter->sector = next_sector;
    }
    /* read raw initial block at (18,1) */
    base_debug("Reading dirent from (18,%d), offset %02x\n",
            iter->sector, iter->offset);
    d64_block_read(iter->d64, buffer, D64_DIR_TRACK, iter->sector);
    /* convert to dirent */
    d64_dirent_read(&(iter->dirent), buffer + iter->offset);
    iter->index++;
    return (bool)(iter->dirent.name[0]);
}


/** \brief  Dump info on dirent \a iter on stdout
 *
 * \param[in]   iter    D64 dirent iter
 */
void d64_dirent_iter_dump(const d64_dirent_iter_t *iter)
{
    printf("dir sector = %d, in-sector offset: %02x\n",
            iter->sector, (unsigned int)(iter->offset));
}


/*
 * D64 block iterator methods
 */
#if 0

/** \brief  Check if \a track and \a sector are valid for \a d64
 *
 * Checks \a track and \a sector against limits for \a d64.
 *
 * \param[in]   d64     D64 image
 * \param[in]   track   track number
 * \param[in]   sector  sector number
 *
 * \return  params are valid
 * \throw   T64_ERR_D64_NULL
 * \throw   T64_ERR_D64_TRACK_RANGE
 * \throw   T64_ERR_D64_SECTOR_RANGE
 */
bool d64_block_is_valid(const d64_t *d64, int track, int sector)
{
    int track_max;
    int sector_max;

    if (d64 == NULL) {
        t64_errno = T64_ERR_D64_NULL;
        return false;
    }

    /* get max track number for the current image */
    track_max = (d64->type == D64_TYPE_CBMDOS ? 35 : 40);

    /* check track number */
    if (track < D64_TRACK_MIN || track > track_max) {
        t64_errno = T64_ERR_D64_TRACK_RANGE;
        return false;
    }

    /* check sector number */
    sector_max = d64_track_max_sector(track);
    if (sector < D64_SECTOR_MIN || sector > sector_max) {
        t64_errno = T64_ERR_D64_SECTOR_RANGE;
        return false;
    }

    return true;
}
#endif


/** \brief  Initialize D64 block iterator
 *
 * \param[out]  iter    D64 block iterator
 * \param[in]   d64     D64 image
 * \param[in]   track   track number
 * \param[in]   sector  sector number
 *
 * \return  bool
 */
bool d64_block_iter_init(d64_block_iter_t *iter,
                             d64_t *d64,
                             int track, int sector)
{
    if (d64_block_is_valid(d64, track, sector)) {
        base_debug("d64 okay");
        iter->d64 = d64;
        iter->track = track;
        iter->sector = sector;
        iter->size = D64_BLOCK_SIZE_RAW;
        if (d64_block_read(d64, iter->data, track, sector)) {
            iter->valid = true;
            return true;
        }
    }
    iter->valid = false;
    return false;
}


/** \brief  Move block iterator to the next block
 *
 * \param[in,out]       iter    block iterator
 *
 * \return  true if next block is valid
 *
 * \note    Check t64_errno for unexpected errors if this returns false
 */
bool d64_block_iter_next(d64_block_iter_t *iter)
{
    int next_track = iter->data[D64_BLOCK_TRACK];
    int next_sector = iter->data[D64_BLOCK_SECTOR];

    base_debug("block iter: current: (%d,%d), next: (%d,%d)",
            iter->track, iter->sector, next_track, next_sector);

    /* copy data */
    if (next_track == 0) {
        return false;
    }
    if (!d64_block_read(iter->d64, iter->data, next_track, next_sector)) {
        return false;
    }

    iter->track = next_track;
    iter->sector = next_sector;
    return true;
}


/** \brief  Determine size of file starting at (\a track, \a sector) in \a d64
 *
 * \param[in]   d64     D64 image
 * \param[in]   track   track number of first block of file
 * \param[in]   sector  sector number of first block of file
 *
 * \return  file size or -1 on error
 *
 * \todo    document error codes this could set.
 */
long d64_file_size(d64_t *d64, int track, int sector)
{
    d64_block_iter_t iter;
    long size = 0;

    if (!d64_block_iter_init(&iter, d64, track, sector)) {
        return -1;
    }

    while (d64_block_iter_next(&iter)) {
        size += D64_BLOCK_SIZE_DATA;
    }
    /* the sector number points to the last data byte */
    return size + iter.data[D64_BLOCK_SECTOR] - 1;
}




/** \brief  Initialize D64 directory object
 *
 * \param[out]  dir     D64 directory
 * \param[in]   d64     D64 image providing the directory
 */
void d64_dir_init(d64_dir_t *dir, d64_t *d64)
{
    if (d64 == NULL) {
        fprintf(stderr, "%s:%d: Got NULL for D64!\n",
                __func__, __LINE__);
    }
    dir->d64 = d64;
    memset(dir->diskname, 0, D64_DISKNAME_MAXLEN);
    memset(dir->diskid, 0, D64_DISKID_MAXLEN);
    for (int i = 0; i < D64_DIRENT_MAX; i++) {
        d64_dirent_init(&(dir->entries[i]), d64);
    }
    dir->entry_count = 0;
}


/** \brief  Read directory into \a dir
 *
 * The \a dir should have been initialized with d64_dir_init() beforehand.
 *
 * \param[in,out]   dir D64 directory
 *
 * \return  bool
 */
bool d64_dir_read(d64_dir_t *dir)
{
    long offset;
    uint8_t *bam;
    d64_dirent_iter_t iter;

    /* get pointer to BAM */
    offset = d64_block_offset(D64_BAM_TRACK, D64_BAM_SECTOR);
    bam = dir->d64->data + offset;

    /* get disk name */
    memcpy(dir->diskname, bam + D64_BAM_DISKNAME, D64_DISKNAME_MAXLEN);
    /* get disk id */
    memcpy(dir->diskid, bam + D64_BAM_DISKID, D64_DISKID_MAXLEN);


    /* initialize dirent iter */
    if (!d64_dirent_iter_init(&iter, dir->d64)) {
        base_debug("NO dir entries.\n");
        return true;
    }

    /* iterate over entries */
    do {
        /* copy dirent */
        dir->entries[dir->entry_count++] = iter.dirent;
    } while (d64_dirent_iter_next(&iter));

    return true;
}


/** \brief  Dump directory \a dir on stdout
 *
 * \param[in]   dir D64 directory
 */
void d64_dir_dump(d64_dir_t *dir)
{
    char diskname_buf[D64_DISKNAME_MAXLEN + 1];
    char diskid_buf[D64_DISKID_MAXLEN + 1];

    pet_to_asc_str(diskname_buf, dir->diskname, D64_DISKNAME_MAXLEN);
    pet_to_asc_str(diskid_buf, dir->diskname, D64_DISKID_MAXLEN);

    printf("0 \"%16s\" %5s\n", diskname_buf, diskid_buf);

    for (int i = 0; i < dir->entry_count; i++) {
        d64_dirent_t *dirent = &(dir->entries[i]);
        char filename[CBMDOS_FILENAME_MAX + 1];

        pet_to_asc_str(filename, dirent->name, CBMDOS_FILENAME_MAX);

        printf("%-5d \"%s\" %c%s%c\n",
                (int)(dirent->blocks),
                filename,
                dirent->filetype & CBMDOS_CLOSED_MASK ? ' ' : '*',
                cbmdos_filetype_str(dirent->filetype),
                dirent->filetype & CBMDOS_LOCKED_MASK ? '<' : ' ');
    }
    printf("%d blocks free.\n", d64_blocks_free(dir->d64));
}
