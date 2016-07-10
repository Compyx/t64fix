/* vim: set et ts=4 sw=4 sts=4 fdm=marker syntax=c.doxygen : */

/** @file   optparse.h
 * @brief   Header for optparse
 */

#ifndef HAVE_OPTPARSE_H
#define HAVE_OPTPARSE_H


/** @brief  optparse_exec() exit codes
 */
enum {
    OPT_EXIT_ERROR = -1,        /**< error during parsing */
    OPT_EXIT_HELP = -2,         /**< --help encountered and handled */
    OPT_EXIT_VERSION = -3       /**< --version encountered and handled */
};

/** @brief  Option type enum
 */
typedef enum {
    OPT_BOOL,   /**< boolean option */
    OPT_INT,    /**< integer option (needs long to store its value) */
    OPT_STR     /**< string option */
} option_type_t;



/** @brief  Option declaration type
 */
typedef struct option_decl_s {
    int             name_short; /**< short option name */
    const char *    name_long;  /**< long option name */
    void *          value;      /**< target of option value */
    int             type;       /**< option requires an argument */
    const char *    desc;       /**< option description for --help */
} option_decl_t;


int             optparse_init(option_decl_t *option_list,
                              const char *name,
                              const char *version);
int             optparse_exec(int argc, char *argv[]);
void            optparse_exit(void);
const char **   optparse_args(void);


#endif
