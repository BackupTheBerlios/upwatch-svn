/*
 * strtok_r(3)
 * 20020927 entropy@tappedin.com
 * public domain.  no warranty.  use at your own risk.  have a nice day.
 */

static char software_version[] = "$Id: strtok_r.c,v 1.4 2002/11/17 11:28:12 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include "replacements.h"

char *
strtok_r(char *str, const char *sep, char **lasts)
{
	char *p;

	if (str == NULL) {
		str = *lasts;
	}
	if (str == NULL) {
		return NULL;
	}
	str += strspn(str, sep);
	if ((p = strpbrk(str, sep)) != NULL) {
		*lasts = p + 1;
		*p = '\0';
	} else {
		*lasts = NULL;
	}
	return str;
}
