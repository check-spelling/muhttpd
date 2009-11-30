#include "flags.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int setenv(const char *envname, const char *envval, int overwrite) {
	int l1, l2;
	char *p;

	/* Get current value of envname. */
	p = getenv(envname);
	if(p && !overwrite) {
		/* Environment variable envname already exists
		 * and overwrite is false; we're done. */
		return 0;
	}

	l1 = strlen(envname);
	l2 = strlen(envval);
	p = (char*) malloc(l1 + l2 + 2);
	if(!p) return -1;

	memcpy(p, envname, l1);
	p[l1] = '=';
	memcpy(p + l1 + 1, envval, l2);
	p[l1 + l2 + 1] = 0;

	/* Update environment. */
	return putenv(p);
}
