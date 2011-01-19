/*
** strings.h -- Local strings routines.
*/
#ifdef NOSTRINGS
#define NOSTRDUP
#define NOSTRNCASECMP
#define NOSTRDUP
#define NOSTRSTR
#endif

#ifdef NOSTRDUP
char *strdup(/* char *p */);
#endif

#ifdef NOSTRNCASECMP
int strncasecmp(/* char *p, *q; int n */);
#endif

#ifdef NOSTRTOK
char *strtok(/* char *input, *separators */);
#endif

#ifdef NOSTRSTR
char *strstr(/* char *string, *substr */);
#endif


/* $Header: /team/project/ssmtp/src/RCS/string_ext.h,v 1.1 1992/11/16 20:19:47 davecb Exp $ */
/*
 * $Log: string_ext.h,v $
 * Revision 1.1  1992/11/16  20:19:47  davecb
 * Initial revision
 *
 * Revision 1.1  1992/06/09  16:11:33  davecb
 * Initial revision
 *
 *
 */

