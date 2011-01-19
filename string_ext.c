/*
** strings.c -- Local strings routines.
*/
#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include "string_ext.h"

#ifdef NOSTRDUP
 char *
strdup(p) char *p; {
	char	*q, *malloc(), *strcpy();

	if ((q= malloc((unsigned)strlen(p)+1)) == NULL) {
		return NULL; 
	}
	else {
		return strcpy(q,p);
	}
}
#endif

#ifdef NOSTRNCASECMP
 int
strncasecmp(p,q,n) char *p, *q; int n; {
	int	i;
	char	c,d;

	for (i=0; i < n; i++,p++,q++) {
		if (*p != *q) {
			if (isascii(c= *p) && isascii(d= *q)) {
				if (tolower(c) != tolower(d)) {
					return c-d;
				}
			}
			else {
				return c-d;
			}
		}
	}
	return 0;
}
#endif

#ifdef NOSTRTOK
 char *
strtok(input,seperators) char *input, *seperators; {
	static char *at;
	char	*p, *q, *strchr();

	if (input != NULL) {
		at = input;
	}
	for (p= at; *p; p++) {
		if (strchr(seperators,*p) != NULL) {
			/* Found one. */
			*p = '\0';
			q = at;
			at = p+1;
			return q;
		}
	}
	if (p == at) {
	        return NULL;
	}
	else {
	        return at;
	}
	/*NOTREACHED*/
}
#endif

#ifdef NOSTRSTR
 char *
strstr(string,substr) char *string, *substr; {
	char	*p, *strchr();

	for (p= strchr(string,*substr); p && *p; p++) {
		if (strncmp(p,substr,strlen(substr))==0) {
			return p;
		}
	}
	return NULL;
}
#endif


#ifndef lint
static char *rcsid = "$Header: /team/project/ssmtp/src/RCS/string_ext.c,v 1.1 1992/11/16 20:20:02 davecb Exp $";
#endif


/*
 * $Log: string_ext.c,v $
 * Revision 1.1  1992/11/16  20:20:02  davecb
 * Initial revision
 *
 * Revision 1.1  1992/06/09  16:11:31  davecb
 * Initial revision
 *
 *
 */

