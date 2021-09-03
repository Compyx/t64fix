/** \file   t64.h
 * \brief   T64 image handling - header
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

#ifndef HAVE_T64_H
#define HAVE_T64_H

#include <stdint.h>
#include <stdbool.h>
#include "t64types.h"

t64_image_t *   t64_open(const char *path, int quiet);
void            t64_free(t64_image_t *image);
int             t64_verify(t64_image_t *image, int quiet);
void            t64_dump(const t64_image_t *image);
bool            t64_write(t64_image_t *image, const char *path);
t64_image_t *   t64_create(const char *path,
                           const char **args,
                           int nargs,
                           bool quiet);

#endif
