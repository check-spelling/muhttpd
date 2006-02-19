#include "flags.h"
#include <stdlib.h>

int clearenv(void) {
	extern char **environ;

	/* We would like to free previously set environment variables here,
	 * but at least FreeBSD 5.1 doesn't let us */

	/* Create empty environment */
	environ = malloc(sizeof(char*));
	*environ = NULL;

	return 0;
}
