/** \file   petasc.c
 * \brief   PETSCII to ASCII conversion and vice versa
 *
 * No conversion to 8-bit ASCII codes takes place. 8-bit ASCII is usually
 * Windows' Latin-1 character set, which is non-portable.
 *
 * Translations are not 'one-on-one' (need better term): so calling
 * pet_to_asc() on a value returned by asc_to_pet() may not return the
 * original value passed to asc_to_pet(). For example, ASCII has CR, LF, FF
 * while PETSCII has just CR, so pet_to_asc(asc_to_pet(0x0a)) will return
 * 0x0d, not 0x0a.
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



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "cbmdos.h"

#include "petasc.h"



/** \brief  PETSCII to ASCII translation table
 */
static const uint8_t pet_to_asc_table[256] = {
    /* $00 - $0f */
    0x00, 0x01, 0x02, 0x1b, /* $03: PETSCII STOP -> ASCII Escape */
    0x04, 0x05, 0x06, 0x07,
    0x14,   /* 08: PET disable C=-shift -> ASC Shift Out / X-On */
    0x15,   /* 09: PET enable C=-shift -> ASC Shift In / X-Off */
    0x0a, 0x0b, 0x0c,
    0x0d,    /* 0d: PET CR == ASC CR */
    0x0e, 0x0f,

    /* $10 - $1f */
    0x10, 0x11, 0x12, 0x13,
    0x08,   /* 14: PET Delete -> ASC Backspace */
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,

    /* $20 - $3f (no conversion needed) */
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

    /* $40 - $5f: invert case */
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,

    /* $60 - $7f: copy of PETSCII $c0-$df */
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,

    /* $80- $9f */
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
    0x8c, 0x0d, /* 8d: PET Shift-Return, no ASCII equivalent -> CR */
    0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
    0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,

    0x20,   /* $a0: inverted space -> space */

    /* $a1-$bf: no conversion */
    0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
    0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,

    /* $c0-$df: invert case */
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0xdb, 0xdd, 0xde, 0xdf,

   /* $e0-$ff: copy of PETSCII $a0-$bf */
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
    0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
};


/** \brief  ASCII to PETSCII translation table
 */
static const uint8_t asc_to_pet_table[256] = {
    /* $00-$0f */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x14, /* $08: ASCII Backspace -> PETSCII Delete */
    0x09, 0x0d, /* $0a: ASCII LF -> PETSCII CR */
    0x0b, 0x0d, /* $0c: ASCII FF -> PETSCII CR */
    0x0d, 0x08, /* $0e: ASCII Shift-Out -> PETSCII Disable CBM-Shift */
    0x09, /* $0f: ASCII Shift-In -> PETSCII Enable CBM-Shift */

    /* $10-$1f */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x03, /* $1b: ASCII Escape -> PETSCII STOP */
    0x1c, 0x1d, 0x1e, 0x1f,

    /* $20 - $3f (no conversion needed) */
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

    /* $40-$5f: upper case ASCII */
    0x40, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
    0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,

    /* $60-$7f: lower case ASCII */
    0x27, /* ASCII backtick => PETSCII single quote */
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,
    0x4d,
    0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,

    /* $80- $9f */
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
    0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,

    /* $a1-$bf: no conversion */
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
    0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,

    /* $c0-$df */
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,

    /* $e0-$ff */
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
    0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};



/** @brief  Illegal characters in file names and paths
 *
 * In UNIX(-like) systems, just about everything is allowed with escaping,
 * except the forward slash, in Windows systems there's quite a few disallowed
 * chars.
 */
static const char *host_illegal_chars =
#ifndef _WIN32
    "/";
#else
    "/\\?%*:|\"<>";
#endif




/** \brief  Translate PETSCII code \a pet to ASCII
 *
 * Untranslatable PETSCII codes are not translated into valid ASCII, especially
 * codes above $7f. So when printing ASCII codes from this function, it may be
 * necessary to use isprint(3) or to print hex values.
 *
 * \param[in]   pet     PETSCII code
 *
 * \return  ASCII code
 */
uint8_t pet_to_asc(uint8_t pet)
{
    return pet_to_asc_table[pet];
}


/** \brief  Translate ASCII code \a asc to PETSCII
 *
 * Untranslatable ASCII codes are not translated to valid PETSCII but just used
 * as-is.
 *
 * \param[in]   asc     ASCII code
 *
 * @return  PETSCII code
 */
uint8_t asc_to_pet(uint8_t asc)
{
    return asc_to_pet_table[asc];
}




/** \brief  Check if character \a ch is allowed in a filename/path on the host
 *
 * \param[in]   ch  character
 *
 * \return  bool
 */
bool is_host_allowed_char(int ch)
{
    const char *t = host_illegal_chars;
    while (*t != '\0' && ch != *t) {
        t++;
    }
    return *t == '\0' ? true : false;
}



/** \brief  Translate at most \a n characters of \a pet to ASCII in \a asc
 *
 * Stores at most \a n + 1 bytes in \a asc. Translation stops at the first 0x0
 * in \a pet, but will never exceed \a n bytes.
 *
 * \param[out]  asc     target ASCII string
 * \param[in]   pet     PETSCII string, optionally 0-terminated
 * \param[in]   n       copy at most this number of characters from \a pet
 */
void pet_to_asc_str(char *asc, const uint8_t *pet, size_t n)
{
    size_t i = 0;
    while (i < n && pet[i] != '\0') {
        int b = pet_to_asc(pet[i]);
        asc[i] = b < 0x80 ? (char)b : '_';
        i++;
    }
    asc[i] = '\0';

}


/** \brief  Translate at most \a n characters of \a asc to PETSCII in \a pet
 *
 * Stores at most \a n bytes in \a pet. If strlen(asc) < n the remaining bytes
 * are set to 0.
 *
 * \param[out]  pet     target PETSCII string
 * \param[in]   asc     ASCII string, optionally 0-terminated
 * \param[in]   n       copy at most this number of characters from \a asc
 */
void asc_to_pet_str(uint8_t *pet, const char *asc, size_t n)
{
    size_t i = 0;

    while (i < n && asc[i] != '\0') {
        pet[i] = asc_to_pet((uint8_t)(asc[i]));
        i++;
    }
    while (i < n) {
        pet[i++] = 0x00;
    }
}


/** \brief  Convert PETSCII filename \a pet to ASCII, optionally with extension
 *
 * Removes padding from PETSCII filename, both normal spaces and 0xa0, converts
 * printable PETSCII characters to ASCII, replaces unprintable and illegal
 * characters to '_' and optionally adds a three character extension if \a ext
 * is not NULL.
 *
 * \param[out]  asc     ASCII filename target (must be at least 21 bytes)
 * \param[in]   pet     PETSCII filename (must be 16 bytes)
 * \param[in]   ext     optional extension (without leading '.')
 */
void pet_filename_to_host(char *asc, const uint8_t *pet, const char *ext)
{
    int lead = 0;
    int trail = CBMDOS_FILENAME_MAX - 1;

    while (lead < CBMDOS_FILENAME_MAX &&
            (pet[lead] == 0x20 || pet[lead] == 0xa0)) {
        lead++;
    }
    while (trail >= 0 && (pet[trail] == 0x20 || pet[trail] == 0xa0)) {
        trail--;
    }
    trail++;    /* trail now indexes to the first padding char, seen from
                   the left */
#if 0
    printf(">> lead=%d, trail=%d\n", lead, trail);
#endif
    if (lead < trail) {
        /* copy filename without padding */
        int i;
        for (i = 0; i < trail - lead; i++) {
            int b = pet_to_asc(pet[lead + i]);
            asc[i] = isprint(b) && is_host_allowed_char(b)
                ? (char)b : '_';
        }
        /* terminate string */
        asc[trail - lead] = '\0';
    } else {
        lead = trail;   /* empty string */
        asc[0] = '\0';
    }
    /* extension? */
    if (ext != NULL && *ext != '\0') {
        asc[trail - lead] = '.';
        memcpy(asc + trail - lead + 1, ext, 3);
        asc[trail - lead + 4] = '\0';
    }
}


/** \brief  Write \a value as PETSCII digits in \a pet
 *
 * \param[out]  pet     memory to store digits
 * \param[in]   value   value to write as PETSCII digits
 * \param[in]   len     maximum size of \a pet
 *
 * \return  number of PETSCII digits written
 */
int write_petscii_digits(uint8_t *pet, int value, size_t len)
{
    int digits;
    /* generate in reverse order */
    uint8_t *p = pet;
    while (value != 0 && (size_t)(p - pet) < len) {
        *p++ = (uint8_t)(value % 10 + '0');
        value /= 10;
    }

    /* remember number of digits printed */
    digits = (int)(p - pet);

    /* reverse characters */
    p--;
    while (p > pet) {
        uint8_t b = *pet;
        *pet = *p;
        *p = b;
        pet++;
        p--;
    }

    return digits;
}
