#include "flags.h"
#include "strndup.h"
#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n) {
	char *ret;

	ret = malloc(n + 1);
	if(!ret) return NULL;
	strncpy(ret, s, n);
	ret[n] = 0;

	return ret;
}
