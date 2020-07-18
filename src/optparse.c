/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen : */

/* \brief   t64fix - a small tool to correct T64 tape image files
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

/** @file   optparse.c
 *
 * @brief   Simplified command line parser
 *
 * This command line parser is a severly simplified version of one I wrote for
 * my assembler project. It only supports short and long options of three types:
 * bool, int, string. Combining short options isn't supported, neither is the
 * --option=VALUE syntax, all options that need an argument expect it to be in
 * the next argv element.
 *
 * Exit codes of optparse_exec() are a bit funky: on succesful completion it
 * returns the number of command line arguments not used by options or their
 * arguments. If it returns -1, something went wrong, if it returns -2, --help
 * was requested and handled, if it returns -3, --version was requested and
 * handled.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* needed for `bool` typedef */
#include "base.h"

#include "optparse.h"




/** @brief  Initial size of argument list
 *
 * The argument list gets doubled in size whenever it's full
 */
#define ARGLIST_INIT    64


/** @brief  Program name
 *
 * String used in help and version messages
 */
static const char *prg_name = NULL;

/** @brief  Program version
 *
 * String used in version message
 */
static const char *prg_version = NULL;


/** @brief  List of available options
 */
static option_decl_t *options;


/** @brief  Argument list
 *
 * Collection of command line arguments not used as option arguments
 */
static const char **arglist;


/** @brief  Number of elements used in the argument list
 */
static size_t arglist_used;


/** @brief  Number of elements available in the argument list
 */
static size_t arglist_size;


/** @brief  Initialize argument list
 *
 * @return  bool
 */
static int arglist_init(void)
{
    arglist = malloc(sizeof *arglist * ARGLIST_INIT);
    arglist_size = ARGLIST_INIT;
    arglist_used = 0;
    return arglist != NULL ? 1 : 0;
}


/** @brief  Free argument list
 */
static void arglist_free(void)
{
    free(arglist);
}


/** @brief  Add argument to argument list
 *
 * @param   arg argument (element from argv)
 */
static int arglist_add_arg(const char *arg)
{
    if (arglist_used == arglist_size - 1) {
        /* resize arglist */
        const char **tmp = realloc(arglist, sizeof *tmp * arglist_size * 2);
        if (tmp == NULL) {
            return 0;
        }
        arglist = tmp;
        arglist_size *= 2;
    }
    arglist[arglist_used++] = arg;
    return 1;
}


/** @brief  Find option by \a name_short short or \a name_long
 *
 * @param   name_short  short option name
 * @param   name_long   long option name
 *
 * @return  option or NULL when not found
 */
static option_decl_t *find_option(int name_short, const char *name_long)
{
    option_decl_t *opt = options;

    while (opt->name_short != 0 || opt->name_long != NULL) {
        if ((name_short != 0 && opt->name_short == name_short) ||
                (name_long != NULL && strcmp(opt->name_long, name_long) == 0)) {
            return opt;
        }
        opt++;
    }
    return NULL;
}

/** @brief  Handle \a option
 *
 * Handles \a option. Returns delta in argv, 0 for boolean options, 1 for
 * non-boolean (int/str) options.
 *
 * @param   option  option record
 * @param   arg     optional argument for option
 *
 * @return  delta to add to index in argv (0 or 1) or -1 on error
 */
static int handle_option(option_decl_t *option, const char *arg)
{
    char *endptr;
    int delta = 0;

    switch (option->type) {
        case OPT_BOOL:
            *((bool *)(option->value)) = 1;
            break;
        case OPT_INT:
            if (arg == NULL) {
                return -1;
            }
            errno = 0;
            *((long *)(option->value)) = strtol(arg, &endptr, 0);
            /* check for invalid crap */
            if (endptr == arg || errno == ERANGE) {
                fprintf(stderr,
                        "%s: failed to convert option argument to int: '%s'\n",
                        prg_name, arg);
                return -1;
            }

            delta = 1;
            break;
        case OPT_STR:
            if (arg == NULL) {
                return -1;
            }
            *((const char **)(option->value)) = arg;
            delta = 1;
            break;
        default:
            fprintf(stderr, "%s: illegal option type %d\n",
                    prg_name, option->type);
            return -1;
    }
    return delta;
}


/** @brief  Print \a option information on stdout
 *
 * TODO:    Needs work, for example adding ARG and perhaps ARG description,
 *          not to mention proper indentation, not using fucking TABS's
 *
 * @param   option  option declaration
 *
 * @return  number of characters printed on stdout
 */
static int print_option(option_decl_t *option)
{
    return printf("  -%c, --%-20s%s\n", option->name_short, option->name_long,
                option->desc);
}

/** @brief  Display usage message and options list
 */
void optparse_help(void)
{
    option_decl_t *opt = options;

    printf("Usage: %s [options] [arguments]\n\n", prg_name);
    printf("Options:\n");
    printf("  --help                    display help\n");
    printf("  --version                 display version information\n");
    for (opt = options; opt->name_short != 0 && opt->name_long != NULL; opt++) {
        print_option(opt);
    }
}


/** @brief  Display program name and version
 */
static void optparse_version(void)
{
    printf("%s %s\n", prg_name, prg_version);
}


/** @brief  Initialize the option parser
 *
 * @param   option_list list of options available
 * @param   name        program name for use in messages
 * @param   version     program version for use in messages
 *
 * @return  bool
 */
int optparse_init(option_decl_t *option_list,
                   const char *name,
                   const char *version)
{
    int result;
    options = option_list;
    prg_name = name;
    prg_version = version;

#ifdef OPTPARSE_DEBUG
    printf("%s:%d: initializing argument list .. ", __FILE__, __LINE__);
#endif
    result = arglist_init();
#ifdef OPTPARSE_DEBUG
    if (result) {
        printf("OK\n");
    } else {
        printf("failed\n");
    }
#endif
    return result;
}


/** @brief  Execute option parser
 *
 * @param   argc    argument count (from main)
 * @param   argv    argument vector (from main)
 *
 * @return  number of arguments remaining
 */
int optparse_exec(int argc, char *argv[])
{
    int i;
#ifdef OPTPARSE_DEBUG
    printf("%s:%d: argc = %d, argv[0] = '%s'\n",
            __FILE__, __LINE__, argc, argv[0]);
#endif
    /* check --help and --version */
    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0) {
#ifdef OPTPARSE_DEBUG
            printf("%s:%d: got --help\n", __FILE__, __LINE__);
#endif
            optparse_help();
            return OPT_EXIT_HELP;
        } else if (strcmp(argv[1], "--version") == 0) {
#ifdef OPTPARSE_DEBUG
            printf("%s:%d: got --version\n", __FILE__, __LINE__);
#endif
            optparse_version();
            return OPT_EXIT_VERSION;
        }
    }

    /* parse argument list */
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        int delta;

        if (arg[0] != '-') {
            /* not an option */
#ifdef OPTPARSE_DEBUG
            printf("%s:%d: found argument: '%s'\n", __FILE__, __LINE__, arg);
#endif
            if (!arglist_add_arg(arg)) {
                return OPT_EXIT_ERROR;
            }
        } else {
            option_decl_t *opt;
#ifdef OPTPARSE_DEBUG
            printf("%s:%d: found possible option: '%s'\n",
                    __FILE__, __LINE__, arg);
#endif
            if (arg[1] == '-') {
                /* long option */
                opt = find_option(0, arg + 2);
            } else {
                opt = find_option(arg[1], NULL);
            }
            if (opt == NULL) {
                fprintf(stderr, "%s: unknown option '%s', continuing\n",
                        prg_name, arg);
                /* don't just complain, do something (thanks iAN) */
                return OPT_EXIT_ERROR;
            }
            delta = handle_option(opt, argv[i + 1]);
            if (delta < 0) {
                return OPT_EXIT_ERROR;
            }
            i += delta;
        }
    }


    return (int)arglist_used;
}


/** @brief  Clean up
 */
void optparse_exit(void)
{
    arglist_free();
}


/** @brief  Get argument list
 *
 * This returns a pointer to a list of elements of argv which were not used
 * during parsing. Do NOT free the pointers in this list, they are copies of
 * pointers in argv, and as such will be cleaned up when the program exits.
 *
 * The list is not NULL-terminated, so use the value returned by optparse_exec()
 * for the list size.
 *
 * @return  list of arguments
 */
const char **optparse_args(void)
{
    return arglist;
}

