#ifndef TYPE_H
#define TYPE_H
#include "config.h"

/** Type description */
struct file_type {
	char *mime_name;
	char *handler;
};

/** A type, extension pair (links file extensions to file types) */
struct type_item {
	struct file_type *type;
	size_t extension_length;
	char *extension;
	struct type_item *next;
};

/** List of types */
struct type_list {
	struct file_type *type;
	struct type_list *next;
};

struct file_type *get_file_type(const char *filename);
struct file_type *register_file_type(const char *name, const char *handler);
struct file_type *associate_type_suffix(const struct file_type *type, 
	const char *suffix);
struct file_type *get_type_by_mime_name(const char *name);
struct file_type *set_type_handler(struct file_type *type, const char *handler);

#endif /* TYPE_H */
