#ifndef MEMORY_H
#define MEMORY_H
#include <stdlib.h>

/** Allocate memory for one item of the specified type */
#define new(T) (T*) malloc(sizeof(T))

#endif /* MEMORY_H */
