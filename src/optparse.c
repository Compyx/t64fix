/** @file   optparse.c
 * \brief   Self-contained command line parser
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This command line parser is a severly simplified version of one I wrote for
 * my assembler project.
 *
 * It only supports short and long options of three types:
 *  - bool
 *  - integer
 *  - string
 * The bool option will set its result to 1 when encountered, so the user is
 * expected to set the result to 0 before calling the parser.
 *
 * Combining short options isn't supported (yet), neither is the
 * `--option=VALUE` syntax (yet), all options that need an argument expect it
 * to be in the next argv element.
 *
 * Exit codes of optparse_exec() are a bit funky:
 * - On succesful completion it returns the number of command line arguments not
 *   used by options or their arguments, this is the number of elements returned
 *   by optparse_args(). It can be 0 if no non-option arguments were found.
 * - OPT_EXIT_ERROR (-1) if something went wrong.
 * - OPT_EXIT_HELP (-2) if --help was encountered.
 * - OPT_EXIT_VERSION (-3) if --version was encountered.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "optparse.h"


/** \brief  Initial size of argument list
 *
 * The argument list gets doubled in size whenever it's full
 */
#define ARGLIST_INIT    64


/*
 * Static variables
 */

/** \brief  Program name
 *
 * String used in help and version messages
 */
static const char *prg_name = NULL;

/** \brief  Program version
 *
 * String used in version message
 */
static const char *prg_version = NULL;

/** \brief  Prologue function for --help
 */
static void (*prologue_cb)(void);

/** \brief  List of available options
 */
static const option_decl_t *options;

/** \brief  Argument list
 *
 * Collection of command line arguments not used as option arguments
 */
static const char **arglist;

/** \brief  Number of elements used in the argument list
 */
static size_t arglist_used;


/** \brief  Number of elements available in the argument list
 */
static size_t arglist_size;



/*
 * Private API
 */

/** \brief  Print memory allocation error message and call abort(3)
 *
 * Prints error message on stderr and then calls abort in order to allow use of
 * a debugger.
 *
 * \param[in]   n   number of bytes requested that triggered the error
 */
static void opt_err_alloc(size_t n)
{
    fprintf(stderr, "optparse: failed to allocate %zu bytes, aborting.\n", n);
    abort();
}


/** \brief  Allocate memory
 *
 * Allocate \a n bytes of memory on the heap, call abort() on failure.
 *
 * \param[in]   n   number of bytes to allocate
 */
static void *opt_malloc(size_t n)
{
    void *p = malloc(n);

    if (p == NULL) {
        opt_err_alloc(n);
    }
    return p;
}


/** \brief  Reallocate memory
 *
 * Rellocate \a p to \a n bytes, call abort() on failure.
 *
 * \param[in]   p   block of memory to resize
 * \param[in]   n   new size
 */
static void *opt_realloc(void *p, size_t n)
{
    void *t = realloc(p, n);

    if (t == NULL) {
        opt_err_alloc(n);
    }
    return t;
}


/** \brief  Initialize argument list
 */
static void arglist_init(void)
{
    arglist = opt_malloc(sizeof *arglist * ARGLIST_INIT);
    arglist_size = ARGLIST_INIT;
    arglist_used = 0;
}


/** \brief  Free argument list
 */
static void arglist_free(void)
{
    free(arglist);
}


/** \brief  Add argument to argument list
 *
 * \param[in]   arg argument (element from argv)
 */
static void arglist_add_arg(const char *arg)
{
    if (arglist_used == arglist_size - 1) {
        /* resize arglist */
        arglist_size *= 2;
        arglist = opt_realloc(arglist, sizeof *arglist * arglist_size);
    }
    arglist[arglist_used++] = arg;
}


/** \brief  Find option by \a name_short short or \a name_long
 *
 * \param[in]   name_short  short option name
 * \param[in]   name_long   long option name
 *
 * \return  option or NULL when not found
 */
static const option_decl_t *find_option(int name_short, const char *name_long)
{
    const option_decl_t *opt = options;

    while (opt->name_short != 0 || opt->name_long != NULL) {
        if ((name_short != 0 && opt->name_short == name_short) ||
                (name_long != NULL && strcmp(opt->name_long, name_long) == 0)) {
            return opt;
        }
        opt++;
    }
    return NULL;
}


/** \brief  Handle an option
 *
 * Handles \a option.
 *
 * Returns delta in argv, 0 for boolean options, 1 for non-boolean (int/str)
 * options.
 *
 * \param[in]   option  option record
 * \param[in]   arg     optional argument for option
 *
 * \return  delta to add to index in argv (0 or 1) or -1 on error
 */
static int handle_option(const option_decl_t *option, const char *arg)
{
    char *endptr;
    int delta = 0;

    switch (option->type) {

        case OPT_BOOL:
            /* boolean option, no argument */
            *((bool *)(option->value)) = 1;
            break;

        case OPT_INT:
            /* integer option, single argument */
            if (arg == NULL) {
                if (option->name_short > 0) {
                    fprintf(stderr,
                            "%s: Error: option -%c' requires an integer argument\n",
                            prg_name, option->name_short);
                } else {
                    fprintf(stderr,
                            "%s: Error: option '--%s' requires an integer argument\n",
                            prg_name, option->name_long);
                }
                return -1;
            }
            /* try to convert argument to long int */
            errno = 0;
            *((long *)(option->value)) = strtol(arg, &endptr, 0);
            /* check for invalid crap */
            if (endptr == arg || errno == ERANGE) {
                fprintf(stderr,
                        "%s: Error: failed to convert option argument "
                        "to int: '%s'\n",
                        prg_name, arg);
                return -1;
            }
            delta = 1;
            break;

        case OPT_STR:
            /* string option, single argument */
            if (arg == NULL) {
                if (option->name_short > 0) {
                    fprintf(stderr,
                            "%s: Error: option -%c' requires an argument\n",
                            prg_name, option->name_short);
                } else {
                    fprintf(stderr,
                            "%s: Error: option '--%s' requires an argument\n",
                            prg_name, option->name_long);
                }
                return -1;
            }
            *((const char **)(option->value)) = arg;
            delta = 1;
            break;

        default:
            /* option type not recognized */
            fprintf(stderr, "%s: illegal option type %d\n",
                    prg_name, option->type);
            return -1;
    }
    return delta;
}


/** \brief  Print option information on stdout
 *
 * Print the data of \a option on stdout for the `--help` option.
 *
 * \todo    Needs work, for example adding ARG and perhaps ARG description.
 *
 * \param[in]   option  option declaration
 *
 * \return  number of characters printed on stdout
 */
static int print_option(const option_decl_t *option)
{
    return printf("  -%c, --%-20s%s\n",
                  option->name_short,
                  option->name_long,
                  option->desc);
}

/** \brief  Display usage message and options list
 *
 * Print usage, optional prologue and list of option descriptions.
 */
void optparse_help(void)
{
    const option_decl_t *opt = options;

    printf("Usage: %s [options] [arguments]\n\n", prg_name);
    if (prologue_cb != NULL) {
        prologue_cb();
    }
    printf("Options:\n");
    printf("  --help                    display help\n");
    printf("  --version                 display version information\n");
    for (opt = options; opt->name_short != 0 && opt->name_long != NULL; opt++) {
        print_option(opt);
    }
}


/** \brief  Display program name and version
 */
static void optparse_version(void)
{
    printf("%s %s\n", prg_name, prg_version);
}


/** \brief  Initialize the option parser
 *
 * \param[in]   option_list list of options available
 * \param[in]   name        program name for use in messages
 * \param[in]   version     program version for use in messages
 *
 * \return  bool
 */
void optparse_init(const option_decl_t *option_list,
                   const char *name,
                   const char *version)
{
    options = option_list;
    prg_name = name;
    prg_version = version;

#ifdef OPTPARSE_DEBUG
    printf("%s:%d: initializing argument list .. ", __FILE__, __LINE__);
#endif
    arglist_init();
}


/** \brief  Execute option parser
 *
 * \param[in]   argc    argument count (from main)
 * \param[in]   argv    argument vector (from main)
 *
 * \return  number of arguments remaining
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
            arglist_add_arg(arg);
        } else {
            const option_decl_t *opt;
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


/** \brief  Clean up memory used by parser
 *
 * Free the non-option argument list.
 *
 * This needs to be called when the user is done with the argument list.
 */
void optparse_exit(void)
{
    arglist_free();
}


/** \brief  Get argument list
 *
 * This returns a pointer to a list of elements of argv which were not used
 * during parsing.
 *
 * Do NOT free the pointers in this list, they are copies of pointers in argv,
 * and as such will be cleaned up when the program exits.
 *
 * The list is not NULL-terminated, so use the value returned by optparse_exec()
 * for the list size.
 *
 * \return  list of arguments
 */
const char **optparse_args(void)
{
    return arglist;
}


/** \brief  Set the prologue function
 *
 * Set the function to print text between the usage message and the options list.
 *
 * \param[in]   func    function to call during `--help` handling
 */
void optparse_set_prologue(void (*func)(void))
{
    prologue_cb = func;
}
