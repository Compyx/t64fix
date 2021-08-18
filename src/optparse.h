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

/** 'file   optparse.h
 * \brief   Header for optparse
 */

#ifndef HAVE_OPTPARSE_H
#define HAVE_OPTPARSE_H


/** \brief  optparse_exec() exit codes
 */
enum {
    OPT_EXIT_ERROR = -1,        /**< error during parsing */
    OPT_EXIT_HELP = -2,         /**< --help encountered and handled */
    OPT_EXIT_VERSION = -3       /**< --version encountered and handled */
};

/** \brief  Option type enum
 */
typedef enum {
    OPT_BOOL,   /**< boolean option */
    OPT_INT,    /**< integer option (needs long to store its value) */
    OPT_STR     /**< string option */
} option_type_t;


/** \brief  Error coce for the parser
 */
enum {
    OPT_ERR_MISSING_ARG,    /**< option requires argument, but none was given */
    OPT_ERR_INVALID_ARG     /**< the argument for the option is invalid */
};



/** \brief  Option declaration type
 */
typedef struct option_decl_s {
    int             name_short; /**< short option name */
    const char *    name_long;  /**< long option name */
    void *          value;      /**< target of option value */
    int             type;       /**< option requires an argument */
    const char *    desc;       /**< option description for --help */
} option_decl_t;


int             optparse_init(const option_decl_t *option_list,
                              const char *name,
                              const char *version);
int             optparse_exec(int argc, char *argv[]);
void            optparse_exit(void);
const char **   optparse_args(void);
void            optparse_help(void);
void            optparse_set_prologue(void (*func)(void));
char *          optparse_strerror(int err);

#endif
