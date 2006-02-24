#include "flags.h"
#include <stdlib.h>

extern char **environ;

int clearenv(void) {
	/* We would like to free previously set environment variables here,
	 * but at least FreeBSD 5.1 doesn't let us */

	/* Create empty environment */
	environ = malloc(sizeof(char*));
	if(environ) {
		*environ = NULL;
	}

	return 0;
}
