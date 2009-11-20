#include "flags.h"
#define CONFIG_C
#include "config.h"
#include "memory.h"
#ifdef ENABLE_SSL
#include "ssl.h"
#endif
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 16384
#define MAX_TOKEN_SIZE 512

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifndef PIDFILE
#define PIDFILE "/var/run/muhttpd.pid"
#endif

struct muhttpd_config *current_config = NULL;

/** Get the next token, or NULL if no more tokens are found.
 * The token is stored in static storage, so be sure to copy it
 * (e.g. using strdup) if you need to preserve the value.
 */
static char *get_next_token(char **str) {
	static char tok[MAX_TOKEN_SIZE], *p;
	int i = 0, n = 0;

	p = *str;

	/* Loop until the end of p is reached. */
	while(p[i]) {
		if(p[i] == '\\') {
			/* Escape sequence; decode. */
			i++;
			switch(p[i]) {
			case ' ':
			case '\t':
			case '\\':
			case '"':
			case '#':
				tok[n++] = p[i++];
				break;
			case 'n':
				i++;
				tok[n++] = '\n';
				break;
			case 'r':
				i++;
				tok[n++] = '\r';
				break;
			case 't':
				i++;
				tok[n++] = '\t';
				break;
			case 'x':
				i++;
				if(isdigit(p[i])) {
					tok[n] = p[i] - '0';
				} else if(isalpha(p[i])) {
					tok[n] = (p[i] & ~32) - '7';
				} else {
					fprintf(stderr, "ERROR: "
						"\\x must be followed "
						"by two hexadecimal digits\n");
					return NULL;
				}
				tok[n] = tok[n] << 4;
				i++;
				if(isdigit(p[i])) {
					tok[n] |= p[i] - '0';
				} else if(isalpha(p[i])) {
					tok[n] |= (p[i] & ~32) - '7';
				} else {
					fprintf(stderr, "ERROR: "
						"\\x must be followed "
						"by two hexadecimal digits\n");
					return NULL;
				}
				i++;
				n++;
				break;
			default:
				/* Invalid escape sequence. */
				if(isgraph(p[i])) {
					fprintf(stderr,
						"ERROR: "
						"Invalid escape sequence: "
						"\\\\%c\n", p[i]);
				} else {
					fprintf(stderr,
						"ERROR: "
						"Invalid escape sequence: "
						"\\\\\\%02x\n", p[i]);
				}
				return NULL;
			}
		} else if(p[i] == '#') {
			/* Comment, skip this line. */
			return NULL;
		} else if(p[i] == '\n' || p[i] == '\r') {
			/* Newline, nothing more to parse. */
			break;
		} else if(isspace(p[i])) {
			/* Whitespace separates tokens. If we don't
			 * have a token yet, ignore whitespace. Otherwise,
			 * return what we have.
			 */
			if(n) break;
			else i++;
		} else {
			/* Regular character; copy literally. */
			tok[n++] = p[i++];
		}
	}
	
	/* Update str. */
	*str = &p[i + 1];

	/* If no characters are in token, return NULL. */
	if(!n) return NULL;

	/* NUL-terminate tok. */
	tok[n++] = 0;

	return tok;
}

#ifndef DISABLE_SETUID
/** user directive */
static struct muhttpd_config *user_directive(char *line,
	struct muhttpd_config *config) {

	char *tok;
	struct passwd *pwd;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid syntax in user directive\n",
			stderr);
		return NULL;
	}

	pwd = getpwnam(tok);
	if(!pwd) {
		fprintf(stderr, "ERROR: User '%s' not found\n", tok);
		return NULL;
	}

	config->uid = pwd->pw_uid;
	if(!config->gid) config->gid = pwd->pw_gid;

	tok = get_next_token(&line);
	if(tok) fputs("WARNING: Stray token after user directive\n",
		stderr);

	return config;
}

/** group directive */
static struct muhttpd_config *group_directive(char *line,
	struct muhttpd_config *config) {

	char *tok;
	struct group *grp;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid syntax in group directive\n",
			stderr);
		return NULL;
	}

	grp = getgrnam(tok);
	if(!grp) {
		fprintf(stderr, "ERROR: Group '%s' not found\n", tok);
		return NULL;
	}

	config->gid = grp->gr_gid;

	tok = get_next_token(&line);
	if(tok) fputs("WARNING: Stray token after group directive\n",
		stderr);

	return config;
}
#endif /* !DISABLE_SETUID */

#ifdef ENABLE_LOGGING
/** logfile directive */
static struct muhttpd_config *logfile_directive(char *line,
	struct muhttpd_config *config) {

	char *tok;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid syntax in logfile directive\n",
			stderr);
		return NULL;
	}

	config->logfile = fopen(tok, "a");
	if(!config->logfile) {
		fprintf(stderr,
			"ERROR: Unable to append to logfile (%s)\n",
			tok);
		perror("fopen");
		return NULL;
	}

	tok = get_next_token(&line);
	if(tok) fputs("WARNING: Stray token after logfile directive\n",
		stderr);

	return config;
}
#endif /* ENABLE_LOGGING */

#ifdef ENABLE_PIDFILE
static struct muhttpd_config *pidfile_directive(char *line, struct muhttpd_config *config) {
	char *tok;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid token in pidfile directive\n", stderr);
		return NULL;
	}

	config->pidfile = strdup(tok);

	if(get_next_token(&line))
		fputs("WARNING: Stray token after pidfile directive\n", stderr);

	return config;
}
#endif /* ENABLE_PIDFILE */

#ifdef ENABLE_SSL
static struct muhttpd_config *ssl_cert_directive(char *line, struct muhttpd_config *config) {
	char *tok;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid token in ssl-cert directive\n",
			stderr);
		return NULL;
	}
	if(get_next_token(&line))
		fputs("WARNING: Stray token after ssl-cert directive\n",
			stderr);
	
	if(ssl_set_cert_file(config->ssl_ctx, tok) == 0) return config;
	else {
		ssl_perror("ssl_set_cert_file");
		return NULL;
	}
}

static struct muhttpd_config *ssl_key_directive(char *line, struct muhttpd_config *config) {
	char *tok;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid token in ssl-key directive\n",
			stderr);
		return NULL;
	}
	if(get_next_token(&line))
		fputs("WARNING: Stray token after ssl-key directive\n",
			stderr);
	
	if(ssl_set_key_file(config->ssl_ctx, tok) == 0) return config;
	else {
		ssl_perror("ssl_set_key_file");
		return NULL;
	}
}

static struct muhttpd_config *ssl_port_directive(char *line, struct muhttpd_config *config) {
	char *tok;

	tok = get_next_token(&line);
	if(!tok) {
		fputs("ERROR: Invalid token in ssl-port directive\n",
			stderr);
		return NULL;
	}
	config->ssl_port = (in_port_t) atoi(tok);
	if(get_next_token(&line))
		fputs("WARNING: Stray token after ssl-port directive\n",
			stderr);
	
	return config;
}
#endif /* ENABLE_SSL */

/** Parse configuration line and update configuration */
static struct muhttpd_config *parse_config_line(char *line,
	struct muhttpd_config *config) {

	char *tok, *p;

	tok = get_next_token(&line);

	if(!tok) return config; /* No token, no change */

	if(!strcmp(tok, "port")) {
		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in port directive\n",
				stderr);
			return NULL;
		}
		config->port = (in_port_t) atoi(tok);
		tok = get_next_token(&line);
		if(tok) fputs("WARNING: Stray token after port directive\n",
			stderr);
	} else if(!strcmp(tok, "webdir")) {
		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in webdir directive\n",
				stderr);
			return NULL;
		}
		tok = strdup(tok);
		if(!tok) {
			perror("strdup");
			return NULL;
		}
		config->webdir = tok;
		tok = get_next_token(&line);
		if(tok) fputs("WARNING: Stray token after webdir directive\n",
			stderr);
	} else if(!strcmp(tok, "webroot")) {
		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in webroot directive\n",
				stderr);
			return NULL;
		}
		tok = strdup(tok);
		if(!tok) {
			perror("strdup");
			return NULL;
		}
		config->webroot = tok;
		tok = get_next_token(&line);
		if(tok) fputs("WARNING: Stray token after webroot directive\n",
			stderr);
#ifndef DISABLE_MIME
	} else if(!strcmp(tok, "type")) {
		struct file_type *type;

		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in type directive\n",
				stderr);
			return NULL;
		}

		/* See if file type is already known */
		type = get_type_by_mime_name(tok);

		/* Register type if it wasn't known */
		if(!type) {
			tok = strdup(tok);
			if(!tok) {
				perror("strdup");
				return NULL;
			}
			type = register_file_type(tok, NULL);
			if(!type) {
				perror("register_file_type");
				return NULL;
			}
		}

		while((tok = get_next_token(&line))) {
			tok = strdup(tok);
			if(!tok) {
				perror("strdup");
				return NULL;
			}
			associate_type_suffix(type, tok);
		}
#endif /* !DISABLE_MIME */
#ifdef ENABLE_HANDLERS
	} else if(!strcmp(tok, "handler")) {
		struct file_type *type;

		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in handler directive\n",
				stderr);
			return NULL;
		}

		type = get_type_by_mime_name(tok);
		if(!type) {
			fprintf(stderr,
				"ERROR: Unknown type \"%s\" in handler "
				"directive\n", tok);
			return NULL;
		}

		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in handler directive\n",
				stderr);
			return NULL;
		}

		tok = strdup(tok);
		if(!tok) {
			perror("strdup");
			return NULL;
		}
		if(!set_type_handler(type, tok)) {
			fprintf(stderr, "ERROR: Could not set handler \"%s\""
				" for type \"%s\"\n", tok, type->mime_name);
			return NULL;
		}

		tok = get_next_token(&line);
		if(tok) fputs("WARNING: Stray token after handler directive\n",
			stderr);

		return config;
#endif /* ENABLE_HANDLERS */
	} else if(!strcmp(tok, "include")) {
		tok = get_next_token(&line);
		if(!tok) {
			fputs("ERROR: Invalid syntax in config directive\n",
				stderr);
			return NULL;
		}
		config = read_config_file(tok, config);
		if(!config) fprintf(stderr,
			"ERROR: Error loading configuration from \"%s\"\n",
			tok);

		tok = get_next_token(&line);
		if(tok) fputs("WARNING: Stray token after handler directive\n",
			stderr);

		return config;
	} else if(!strcmp(tok, "index")) {
		while((tok = get_next_token(&line))) {
			p = realloc(config->index, (size_t)
				((config->indices + 1) * sizeof(char*)));
			if(!p) {
				fputs("ERROR: Error adding index", stderr);
				config = NULL;
				break;
			}
			config->index = (void*) p;
			config->index[config->indices] = strdup(tok);
			config->indices++;
		}
#ifndef DISABLE_SETUID
	} else if(!strcmp(tok, "user")) {
		return user_directive(line, config);
	} else if(!strcmp(tok, "group")) {
		return group_directive(line, config);
#endif
#ifdef ENABLE_LOGGING
	} else if(!strcmp(tok, "logfile")) {
		return logfile_directive(line, config);
#endif
#ifdef ENABLE_PIDFILE
	} else if(!strcmp(tok, "pidfile")) {
		return pidfile_directive(line, config);
	} else if(!strcmp(tok, "nopidfile")) {
		config->pidfile = NULL;
		return config;
#endif
#ifdef ENABLE_BACKGROUND
	} else if(!strcmp(tok, "background")) {
		config->background = 1;
	} else if(!strcmp(tok, "foreground")) {
		config->background = 0;
#endif
#ifdef ENABLE_SSL
	} else if(!strcmp(tok, "ssl-cert")) {
		return ssl_cert_directive(line, config);
	} else if(!strcmp(tok, "ssl-key")) {
		return ssl_key_directive(line, config);
	} else if(!strcmp(tok, "ssl-port")) {
		return ssl_port_directive(line, config);
#endif
	} else {
		fprintf(stderr, "ERROR: Configuration directive \"%s\" not "
			"understood\n", tok);
		return NULL;
	}

	return config;
}

/** Read a configuration file and update the configuration */
struct muhttpd_config *read_config_file(const char *file,
	struct muhttpd_config *config) {

	FILE *fp;
	char line[MAX_LINE_LENGTH], *dir, olddir[PATH_MAX];

	if(!getcwd(olddir, PATH_MAX)) {
		perror("getcwd");
		return NULL;
	}

	dir = strdup(file);
	if(chdir(dirname(dir))) {
		free(dir);
		perror(dir);
		return NULL;
	}
	free(dir);

	dir = strdup(file);
	fp = fopen(basename(dir), "r");
	if(!fp) {
		free(dir);
		if(chdir(olddir)) {
			perror(olddir);
		}
		return NULL;
	}
	free(dir);

	/* Read configuration file line by line */
	while(!feof(fp)) {
		if(!fgets(line, MAX_LINE_LENGTH, fp)) break;
		/* Check if we got a newline or EOF at the end */
		if(line[strlen(line) - 1] != '\n' && !feof(fp)) {
			fprintf(stderr, "WARNING: Line too long in %s\n",
				file);
		} else {
			config = parse_config_line(line, config);	
			if(!config) break;
		}
	}

	if(chdir(olddir)) {
		perror(olddir);
	}

	fclose(fp);

	return config;
}

struct muhttpd_config *get_default_config() {
	struct muhttpd_config *config;

	config = new(struct muhttpd_config);
	if(!config) return NULL;

	config->port = 80;
	config->webdir = "/var/www";
	config->webroot = NULL;
#ifndef DISABLE_MIME
	config->known_types = NULL;
	config->known_extensions = NULL;
#endif
	config->indices = 0;
	config->index = NULL;
#ifndef DISABLE_SETUID
	config->uid = 0;
	config->gid = 0;
#endif
#ifdef ENABLE_LOGGING
	config->logfile = NULL;
#endif
#ifdef ENABLE_PIDFILE
	config->pidfile = PIDFILE;
#endif
#ifdef ENABLE_SSL
	config->ssl_port = 443;
#endif

	return config;
}

struct muhttpd_config *get_current_config() {
	return current_config;
}

struct muhttpd_config *set_current_config(const struct muhttpd_config *cfg) {
	if(current_config) free(current_config);
	current_config = (struct muhttpd_config *) cfg;
	return current_config;
}
