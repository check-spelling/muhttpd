#ifndef CONFIG_H
#define CONFIG_H
#include <sys/types.h>
#include "type.h"
#ifdef ENABLE_LOGGING
#include <stdio.h>
#endif

struct muhttpd_config {
	int port;
	char *webdir;
	char *webroot;
#ifndef DISABLE_MIME
	struct type_list *known_types;
	struct type_item *known_extensions;
#endif
	int indices;
	char **index;
#ifndef DISABLE_SETUID
	uid_t uid;
	gid_t gid;
#endif
#ifdef ENABLE_LOGGING
	FILE *logfile;
#endif
#ifdef ENABLE_PIDFILE
	char *pidfile;
#endif
#ifdef ENABLE_BACKGROUND
	int background;
#endif
};

struct muhttpd_config *read_config_file(const char *file,
	struct muhttpd_config *config);

#ifndef CONFIG_C
extern struct muhttpd_config *current_config;
#endif

struct muhttpd_config *get_default_config();
struct muhttpd_config *get_current_config();
struct muhttpd_config *set_current_config(const struct muhttpd_config *cfg);

#endif /* CONFIG_H */
