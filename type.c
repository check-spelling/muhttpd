#include "flags.h"
#include "type.h"
#include "config.h"
#include "memory.h"
#include "globals.h"
#include <string.h>

/** Get the file type based on file name */
struct file_type *get_file_type(const char *name) {

	struct type_item *cur = config->known_extensions;
	size_t l = strlen(name);

	/* Iterate over the list until we find a matching extension */
	while(cur) {
		if(l >= cur->extension_length) {
			if(!strcmp(&name[l - cur->extension_length],
				cur->extension)) {

				/* Found it, return associated type */
				return cur->type;
			}
		}
		cur = cur->next;
	}

	return NULL;
}

/** Register a file type, with optional handler
 * @return a pointer to a struct file_type (can be used in associate_suffix)
 */
struct file_type *register_file_type(const char *name, const char *handler) {
	struct type_list *foo;

	foo = new(struct type_list);
	if(!foo) return NULL;

	foo->type = new(struct file_type);
	if(!foo->type) return NULL;

	foo->type->mime_name = (char*) name;
	foo->type->handler = (char*) handler;
	foo->next = config->known_types;
	config->known_types = foo;

	return foo->type;
}

/** Associate a suffix with a file type
 * @return The file_type struct of the type
 */
struct file_type *associate_type_suffix(const struct file_type *type,
	const char *suffix) {

	struct type_item *foo;

	foo = new(struct type_item);
	if(!foo) return NULL;

	foo->type = (struct file_type*) type;
	foo->extension_length = strlen(suffix);
	foo->extension = (char*) suffix;
	foo->next = config->known_extensions;
	config->known_extensions = foo;

	return (struct file_type*) type;
}

/** Find the file type with the given MIME name */
struct file_type *get_type_by_mime_name(const char *name) {
	struct type_list *cur = config->known_types;

	while(cur) {
		if(!strcmp(cur->type->mime_name, name)) break;
		cur = cur->next;
	}

	if(!cur) return NULL;
	return cur->type;
}

/** Set handler for type */
struct file_type *set_type_handler(struct file_type *type, const char *handler)
	{

	type->handler = (char*) handler;
	return type;
}
