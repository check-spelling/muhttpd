#ifndef CONFIG_H
#define CONFIG_H
#include <sys/types.h>
#include <netinet/in.h>
#include "type.h"
#ifdef ENABLE_LOGGING
#include <stdio.h>
#endif
#ifdef ENABLE_SSL
#include <openssl/ssl.h>
#endif

struct muhttpd_config {
	in_port_t port;
	char *webdir;
	char *webroot;
	int indices;
	char **index;
#ifdef ENABLE_BACKGROUND
	int background;
#endif
#ifdef ENABLE_LOGGING
	FILE *logfile;
#endif
#ifndef DISABLE_MIME
	struct type_list *known_types;
	struct type_item *known_extensions;
#endif
#ifdef ENABLE_PIDFILE
	char *pidfile;
#endif
#ifndef DISABLE_SETUID
	uid_t uid;
	gid_t gid;
#endif
#ifdef ENABLE_SSL
	in_port_t ssl_port;
	SSL_CTX *ssl_ctx;
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
