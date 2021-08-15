/** \file   cbmdos.c
 * \brief   CBM-DOS generics
 *
 */

/*
 * This file is part of zipcode-conv
 *
 *  Copyright (C) 2020  Bas Wassink <b.wassink@ziggo.nl>
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


#include "cbmdos.h"


/** \brief  Get filetype string for \a type
 *
 * \param[in]   type    CBM-DOS file type
 *
 * \return  filetype string or '???'
 */
const char *cbmdos_filetype_str(cbmdos_filetype_t type)
{
    switch (type & CBMDOS_FILETYPE_MASK) {
        case CBMDOS_FILETYPE_DEL:
            return "del";
        case CBMDOS_FILETYPE_SEQ:
            return "seq";
        case CBMDOS_FILETYPE_PRG:
            return "prg";
        case CBMDOS_FILETYPE_USR:
            return "usr";
        case CBMDOS_FILETYPE_REL:
            return "rel";
        default:
            return "???";
    }
}
