/*
 * Copyright (c) 2008; Nicolas Bouliane <nicboul@gmail.com>
 * Copyright (c) 2008; Samuel Jean <peejix@gmail.com>
 * Copyright (c) 2008; Mind4Networks Technologies INC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "options.h"

enum {
    SUCCESS = 0,
    ERR_SYNTAX,	    /* syntax error */
    ERR_DUPLICATE,  /* duplicate item */
    ERR_ERRNO,	    /* unknown error but errno knows */
    ERR_TYPE,	    /* invalid option type */
    ERR_MAN	    /* mandatory option not present */
};

#define break_line(e, l) do { printf("line %i: ", l); return e; } while (0)

void option_dump(struct options *opts)
{
	journal_strace("option_dump");

    int i = 0;
    while (opts[i].tag != NULL) {

	    if (opts[i].type & OPT_STR)
		    journal_notice("option]> %s == %s\n", opts[i].tag, *(char **)(opts[i].value) ? *(char **)(opts[i].value) : "(nil)");

	    else if (opts[i].type & OPT_INT)
		    if (*(int **)(opts[i].value))
			    journal_notice("option]> %s == %i\n", opts[i].tag, **(int **)(opts[i].value));
		    else
			    journal_notice("option]> %s == (nil)\n", opts[i].tag);
	    else
		    journal_notice("option]> %s == err: invalid type\n", opts[i].tag);
	    
	    i++;
    }
}

/* trim(): remove whitespaces from string
 * @str	    -> a pointer to the string to strip
 * $return  -> the string with no spaces
 */
static char *trim(char *str)
{
	journal_strace("trim");

    char *a, *z;

    a = str;
    while (*a == ' ') a++;

    z = a + strlen(a);
    while (*--z == ' ' && (z > a));
    *++z = '\0';

    return a;
}

/* is_comment(): verify wether line is a comment or not
 * @str	    -> the string to verify
 * $return  -> 1 if line is a comment, 0 otherwise
 * The line is considered a comment if the following is found
 * to be the first character beside spaces :
 * 	# OR ; OR [
 */
static int is_comment(char *str)
{
	journal_strace("is_comment");

    char *p;

    p = str;
    while (*p == ' ') p++;
    switch(*p) {
	    case '#':
	    case '[':
	    case ';':
		    return 1;
    }

    return 0;
}

/* try_set_value(): find matching tag, then store value
 * @str	    -> the string associated to the tag
 * @tag	    -> the tag name to find in options struct
 * @opts    -> the options struct
 * $return  -> 0 if no error occured, an ERR_ value otherwise
 *
 * This function is used internaly by parse() everytime it
 * finds a `tag = value' pair in the config file.
 */
static int try_set_value(char *str, char *tag, struct options *opts)
{
	journal_strace("try_set_value");

    int i = 0;
    for (; opts[i].tag != NULL; i++) {

	    if (strcmp(opts[i].tag, tag) != 0)
		    continue;

	    if (*(void **)(opts[i].value) != NULL)
		    return ERR_DUPLICATE;

	    if (opts[i].type & OPT_STR) {
		    *(void **)(opts[i].value) = strdup(str);

		    if (*(void **)(opts[i].value) == NULL)
			    return ERR_ERRNO;
	    }
	    else if (opts[i].type & OPT_INT) {
		    *(void **)(opts[i].value) = malloc(sizeof(int));
		    if (*(void **)(opts[i].value) == NULL)
			    return ERR_ERRNO;

		    **(int **)(opts[i].value) = strtoul(str, NULL, 0);

		    /* If no conversion was performed, strtoul() returns a
		     * value of zero. If the converted value overflows,
		     * strtoul() returns ULONG_MAX.
		     *
		     * XXX What about the 0 case ?
		     */
		    switch (**(int **)(opts[i].value)) {
			    case 0:
			    case ULONG_MAX:
				    return ERR_ERRNO;
		    }

	    } else {
		    /* the caller MUST specify a valid option type */
		    return ERR_TYPE;
	    }			
    }

    return 0;
}

/* parse(): parse the file pointer to populate options struct
 * @opts    -> the options struct
 * @fp	    -> a pointer to the file
 * $return  -> 0 if no error occured, an ERR_ value otherwise
 *
 * MAX_LEN is the maximum length allowed for a line.
 *
 * This function retreives each line of the given file, strip off
 * comments and bail out on stuborn syntax error. For each valid
 * line, it asks try_set_value() to store the value inside the
 * options struct.
 */
#define MAX_LEN 256
static int parse(struct options *opts, FILE *fp)
{
	journal_strace("parse");

    char *p, arg[MAX_LEN];
    int line = 0, ret = 0, err = 0;

    memset(arg, 0, MAX_LEN);

    /* XXX What if a line is longer than 256 ? */
    while (!feof(fp) && (ret = fscanf(fp, "\n%255[^\n]\n", arg)) != 0) {

	    line++;

	    if (is_comment(arg))
		    continue;

	    p = strstr(arg, "=");
	    if (p == NULL)
		    break_line(ERR_SYNTAX, line);

	    *p = '\0'; p++;

	    p = trim(p);
	    if (*p == 0)
		    break_line(ERR_SYNTAX, line);

	    err = try_set_value(p, trim(arg), opts);
	    if (err > 0)
		    break_line(err, line);
    }

    if (ret == 0) /* fscanf() failed */
	    return ERR_ERRNO;

    return 0;
}

/* sanity_check(): final option type lookup
 * @opts    -> pointer to the options structure
 * $return  -> 1 if option types are satisfied, 0 otherwise
 */
static int sanity_check(const struct options *opts)
{
	journal_strace("sanity_check");

    int i = 0;
    for (; opts[i].tag != NULL; i++)
	    if ((opts[i].type & OPT_MAN) &&
		    *(void **)(opts[i].value) == NULL) {
		    journal_notice("'%s' is missing from configuration\n",
			    opts[i].tag);
		    return ERR_MAN;
	    }

    return SUCCESS;
}

/* option_free(): free allocated memory
 * @opts    -> pointer to the options structure
 * $return  -> nothing
 *
 * Use this function to release memory allocated by strdup() and malloc()
 * in the try_set_value() function when you no longer need them.
 */
void option_free(struct options *opts)
{
	journal_strace("option_free");

    int i = 0;
    for (; opts[i].tag != NULL; i++)
	    free(*(void **)(opts[i].value));
}

/* option_parse(): parse a configuration file
 * @opts    -> the options struct
 * @path    -> an absolute pathname to the file
 * $return  -> 1 on success, 0 otherwise
 *
 * This is the exported function to allow caller to parse a configuration
 * file and populate the provided options structure.
 */
int option_parse(struct options *opts, char *path)
{
	journal_strace("option_parse");

    FILE *fp;
    int i;

    if ((fp = fopen(path, "r")) == NULL) {
	    journal_notice("%s: %s\n", path, strerror(errno));
	    return 1;
    }
    
    for (i = 0; opts[i].tag != NULL; i++)
	    *(void **)opts[i].value = NULL;

    /* XXX This is a proof-of-concept */
    switch(parse(opts, fp)) {
	    case ERR_SYNTAX:
		    journal_notice("option]> syntax error\n");
		    fclose(fp);
		    return ERR_SYNTAX;

	    case ERR_DUPLICATE:
		    journal_notice("option]> duplicate\n");
		    fclose(fp);
		    return ERR_DUPLICATE;

	    case ERR_ERRNO:
		    perror("unexpected error");
		    fclose(fp);
		    return ERR_ERRNO;

	    case ERR_TYPE:
		    journal_notice("option]> invalid option type\n");
		    fclose(fp);
		    return ERR_TYPE;
    }

    fclose(fp);
    return sanity_check(opts);

    /* XXX In all error case, option_free() should be call to free() memory */
}
