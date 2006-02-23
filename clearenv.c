#include "flags.h"
#include <stdlib.h>

/*@-incondefs@*/
/*@null@*/
extern char **environ;

int clearenv(void) {
	environ = NULL;

	return 0;
}
