#include <stdlib.h>
#include <string.h>

/** Concatenate two strings into a newly allocated memory buffer.
 * @param s1 First string
 * @param s2 Second string
 * @return The new string, or NULL if an error occurred.
 */
char *stracat(const char *s1, const char *s2) {
	int l1 = strlen(s1), l2 = strlen(s2), size = l1 + l2 + 1;
	char *str;

	str = (char*) malloc(size);
	if(str) {
		memcpy(str, s1, l1);
		memcpy(str + l1, s2, l2);
		str[size - 1] = 0;
	}

	return str;
}
