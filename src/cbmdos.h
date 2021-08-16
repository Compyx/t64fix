/** \file   cbmdos.h
 * \brief   CBM-DOS generics - header
 *
 */

/*
 *  This file is part of t64fix
 *  Copyright (C) 2016-2021  Bas Wassink <b.wassink@ziggo.nl>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*
 */

#ifndef CBMDOS_H
#define CBMDOS_H

/** \brief  Maximum length of a CBM DOS filename in PETSCII
 */
#define CBMDOS_FILENAME_MAX 16


/** \brief  File type enumerator
 */
typedef enum cbmdos_filetype_e {
    CBMDOS_FILETYPE_DEL = 0,    /**< DELeted file */
    CBMDOS_FILETYPE_SEQ,        /**< SEQuental file */
    CBMDOS_FILETYPE_PRG,        /**< PRoGram file */
    CBMDOS_FILETYPE_USR,        /**< USeR file */
    CBMDOS_FILETYPE_REL         /**< RELative file */
} cbmdos_filetype_t;


/** \brief  Bitmask to filter out file type of a filetype byte
 */
#define CBMDOS_FILETYPE_MASK    0x07

/** \brief  Bitmask to filter out the locked state of a filetype byte
 */
#define CBMDOS_LOCKED_MASK      0x40

/** \brief  Bitmask to filter out the closed state of a filetype byte
 */
#define CBMDOS_CLOSED_MASK      0x80


const char *cbmdos_filetype_str(cbmdos_filetype_t type);

#endif
