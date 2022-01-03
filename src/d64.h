/** \file   d64.h
 * \brief   D64 handling - header
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *  This file is part of t64fix
 *  Copyright (C) 2016-2021  Bas Wassink
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

#ifndef D64_H
#define D64_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cbmdos.h"


/** \brief  Size of a standard 35-track D64 image without error info
 */
#define D64_SIZE_CBMDOS     174848

/** \brief  Sizeo of a 40-track D64 image without error info
 */
#define D64_SIZE_EXTENDED   (D64_SIZE_CBMDOS + 5 * 17 * 256)


/** \brief  Minimum track number for D64 images
 */
#define D64_TRACK_MIN       1

/** \brief  Maximum track number for standard (35-track) D64 images
 */
#define D64_TRACK_MAX       35

/** \brief  Maximum track number for extended (40-track) D64 images
 */
#define D64_TRACK_MAX_EXT   40

/** \brief  Minimum sector number for D64 images
 */
#define D64_SECTOR_MIN       0

/** \brief  Maximum sector number for D64 images
 *
 * Only valid for the first speedzone, tracks 1-17, extra checks via the
 * speedzone table are required for proper checking
 */
#define D64_SECTOR_MAX      20

/** \brief  Size of a raw directory entry
 */
#define D64_DIRENT_SIZE     0x20

/** \brief  Size of extra GEOS data in a raw directory entry
 */
#define D64_DIRENT_GEOS_SIZE    0x06


/** \brief  Size of a raw block (sector)
 *
 * This includes the 2-byte pointer to the next block (track,sector)
 */
#define D64_BLOCK_SIZE_RAW  256

/** \brief  Size of the data section of a block
 */
#define D64_BLOCK_SIZE_DATA 254

/** \brief  Offset in a raw block of the next track
 */
#define D64_BLOCK_TRACK 0

/** \brief  Offset in a raw block of the next sector
 */
#define D64_BLOCK_SECTOR 1

/** \brief  Offset in a raw block of the actual data
 */
#define D64_BLOCK_DATA 2



/*
 * Directory entry offsets
 */

/** \brief  Track number of next dir block
 *
 * Ignored by 1541 ROM
 */
#define D64_DIRENT_DIR_TRACK    0x00

/** \brief  Sector number of next dir block
 */
#define D64_DIRENT_DIR_SECTOR   0x01

/** \brief  Filetype and locked/closed bits
 */
#define D64_DIRENT_FILETYPE     0x02

/** \brief  Track number of first block of file
 */
#define D64_DIRENT_TRACK        0x03

/** \brief  Sector number of first block of file
 */
#define D64_DIRENT_SECTOR       0x04

/** \brief  Filename in PETSCII
 */
#define D64_DIRENT_FILENAME     0x05

/** \brief  Track number of first SSB
 */
#define D64_DIRENT_SSB_TRACK    0x15

/** \brief  Sector number of first SSB
 */
#define D64_DIRENT_SSB_SECTOR   0x16

/** \brief  Record size for REL files
 */
#define D64_DIRENT_REL_LENGTH   0x17

/** \brief  GEOS data
 */
#define D64_DIRENT_GEOS         0x18

/** \brief  LSB of the file size in blocks
 */
#define D64_DIRENT_BLOCKS_LSB   0x1e

/** \brief  MSB of the file size in blocks
 */
#define D64_DIRENT_BLOCKS_MSB   0x1f


/** \brief  Offset in bytes in a D64 of the BAM
 */
#define D64_BAM_OFFSET  0x16500


/** \brief  Track number of the BAM
 */
#define D64_BAM_TRACK   18

/** \brief  Sector number of the BAM
 */
#define D64_BAM_SECTOR   0

/** \brief  Track number of the first directory block
 */
#define D64_DIR_TRACK   18

/** \brief  Sector number of the first directory block
 */
#define D64_DIR_SECTOR   1


/** \brief  D64 types
 *
 */
typedef enum d64_type_e {
    D64_TYPE_CBMDOS,        /**< standard CBM DOS 35 tracks */
    D64_TYPE_SPEEDDOS,      /**< SpeedDOS 40 tracks */
    D64_TYPE_DOLPHINDOS,    /**< DolphinDOS 40 tracks */
    D64_TYPE_PROFDOS,       /**< ProfDOS 40 tracks */
    D64_TYPE_PROLOGICDOS    /**< ProLogic 40 tracks */
} d64_type_t;


/** \brief  D64 speedzone entry
 */
typedef struct d64_speedzone_s {
    int track_min;      /**< first track number of the zone */
    int track_max;      /**< last track number of the zone */
    int sectors;        /**< sectors per track in the zone */
} d64_speedzone_t;


/** \brief  D64 handle
 *
 */
typedef struct d64_s {
    char *      path;   /**< path to image file */
    uint8_t *   data;   /**< binary data */
    size_t      size;   /**< size of data */
    d64_type_t  type;   /**< DOS type */
} d64_t;


/** \brief  D64 dirent
 */
typedef struct d64_dirent_s {
    d64_t * d64;            /**< D64 reference */
    uint8_t     name[CBMDOS_FILENAME_MAX];  /**< PETSCII filename */
    uint8_t     geos[D64_DIRENT_GEOS_SIZE]; /**< GEOS data */
    uint16_t    blocks;     /**< size of the file in blocks */
    size_t      size;       /**< size in bytes */
    uint8_t     filetype;   /**< filetype and locked/closed flags */
    uint8_t     dir_track;  /**< track number of next dir block */
    uint8_t     dir_sector; /**< sector number of next dir block */
    uint8_t     track;      /**< track number of first block of file */
    uint8_t     sector;     /**< sector number of first block of file */
    uint8_t     ssb_track;  /**< track number of first side-sector block */
    uint8_t     ssb_sector; /**< sector number of first side-sector block */
    uint8_t     rel_length; /**< size of relative file record */
} d64_dirent_t;


/** \brief  D64 dirent iterator
 */
typedef struct d64_dirent_iter_s {
    d64_t *d64;             /**< reference to D64 */
    d64_dirent_t dirent;    /**< directory entry */
    int sector;                 /**< sector number in track 18 */
    int offset;                 /**< offset in current dir sector */
    int index;                  /**< dirent index in d64 */
} d64_dirent_iter_t;


/** \brief  Size of a D64 disk name in PETSCII
 */
#define D64_DISKNAME_MAXLEN    16

/** \brief  Size of a D64 disk ID in PETSCII
 *
 * This includes the DOS type at $a5-$a6 and the inverted space at $a4
 */
#define D64_DISKID_MAXLEN      5

/** \brief  Track number of first directory block */
#define D64_BAM_DIR_TRACK   0x00

/** \brief  Sector number of first directory block */
#define D64_BAM_DIR_SECTOR  0x01

/** \brief  DOS version */
#define D64_BAM_DOS_VERSION 0x02

/* $03 is unused, but normally set to $a0, can be used for dirart */

/** \brief  Offset in BAM of the BAM entries for tracks 1-35
 */
#define D64_BAM_TRACKS      0x04

/** \brief  Offset in BAM of the disk name
 */
#define D64_BAM_DISKNAME    0x90

/* $a0 and $a1 are filled with $a0 */

/** \brief  Offset in BAM of the disk ID */
#define D64_BAM_DISKID      0xa2

/* $a4 is unused, usually $a0 */

/** \brief  Offset in BAM of the DOS type
 *
 * The DOS type is two bytes and "2A" for standard CBM DOS
 */
#define D64_BAM_DOS_TYPE    0xa5

/* $a7-$aa is filled with $a0 */

/* $ab-$ff is $00 with normal 35 track images */


/** \brief  Size of a BAM entry for a track
 */
#define D64_BAMENT_SIZE     0x04

/** \brief  Number of sectors free for a track in a BAM entry
 */
#define D64_BAMENT_COUNT    0x00

/** \brief  Bitmap of free sectors for a track in a BAM entry
 */
#define D64_BAMENT_BITMAP   0x01



/** \brief  Maximum number of directory entries for a 1541
 *
 * This is hardcoded in the ROM
 */
#define D64_DIRENT_MAX  144


/** \brief  D64 directory object
 */
typedef struct d64_dir_s {
    d64_t *d64;                             /**< D64 reference */
    uint8_t diskname[D64_DISKNAME_MAXLEN];  /**< PETSCII disk name */
    uint8_t diskid[D64_DISKID_MAXLEN];      /**< PETSCII disk ID + DOS type */
    d64_dirent_t entries[D64_DIRENT_MAX];   /**< directory entries */
    int entry_count;                        /**< number of directory entries */
} d64_dir_t;


/** \brief  D64 block iterator object
 *
 * A block is a raw (256 bytes) sector in a D64 image
 */
typedef struct d64_block_iter_s {
    d64_t * d64;                        /**< D64 image */
    uint8_t data[D64_BLOCK_SIZE_RAW];   /**< current block data */
    size_t  size;                       /**< current block data size */
    int     track;                      /**< current block track number */
    int     sector;                     /**< current block sector number */
    bool    valid;                      /**< iterator is valid */
} d64_block_iter_t;



long d64_block_offset(int track, int sector);
long d64_track_offset(int track);
bool d64_track_is_valid(const d64_t *d64, int track);
void d64_init(d64_t *d64);
void d64_alloc(d64_t *d64, d64_type_t type);
void d64_free(d64_t *d64);
bool d64_read(d64_t *d64, const char *path, d64_type_t type);
bool d64_write(d64_t *d64, const char *path);
void d64_dump_info(const d64_t *d64);
void d64_dump_bam(const d64_t *d64);

bool d64_block_read(const d64_t *d64,
                        uint8_t *buffer,
                        int track, int sector);
bool d64_block_write(d64_t *d64,
                         const uint8_t *buffer,
                         int track, int sector);


int d64_blocks_free(d64_t *d64);

void d64_dirent_init(d64_dirent_t *dirent, d64_t *d64);
void d64_dirent_read(d64_dirent_t *dirent, const uint8_t *data);


bool d64_dirent_iter_init(d64_dirent_iter_t *iter, d64_t *d64);
bool d64_dirent_iter_next(d64_dirent_iter_t *iter);
#if 0
void d64_dirent_iter_read_dirent(d64_dirent_iter_t *iter,
                                     d64_dirent_t *dirent);
#endif
void d64_dirent_iter_dump(const d64_dirent_iter_t *iter);


void d64_dir_init(d64_dir_t *dir, d64_t * d64);
bool d64_dir_read(d64_dir_t *dir);
void d64_dir_dump(d64_dir_t *dir);


bool d64_bament_read(const d64_t *d64, uint8_t *bament, int track);


int d64_track_max_sector(int track);

bool d64_block_is_valid(const d64_t *d64, int track, int sector);

bool d64_block_iter_init(d64_block_iter_t *iter,
                             d64_t *d64,
                             int track, int sector);

bool d64_block_iter_next(d64_block_iter_t *iter);

long d64_file_size(d64_t *d64, int track, int sector);


/*
 * Write support
 */

void d64_bam_init(d64_t *d64);

void d64_new(d64_t *d64);

void d64_set_diskname_asc(d64_t *d64, const char *name);
void d64_set_diskname_pet(d64_t *d64, const uint8_t *name);
void d64_set_diskid_asc(d64_t *d64, const char *id);
void d64_set_diskid_pet(d64_t *d64, const uint8_t *id);

#endif
