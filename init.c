#include "flags.h"
#include "socket.h"
#include "type.h"
#include "config.h"
#include "memory.h"
#include "globals.h"
#include "clearenv.h"
#include "setenv.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#ifdef ENABLE_SSL
#include "ssl.h"
#endif

#ifndef CONFIGFILE
#define CONFIGFILE "/etc/muhttpd/muhttpd.conf"
#endif

extern SOCKET sock;	/* from main.c */
#ifdef ENABLE_SSL
extern SOCKET ssl_sock;	/* from main.c */
#endif

static void sighandler(int num) {
	fprintf(stderr, "Caught signal %d\n", num);
	exit(EXIT_FAILURE);
}

/* Used to clean up exited child processes. */
static void sigchldhandler(int num) {
	/* SIGCHLD may be raised for reasons other than child processes having
	   exited. Also, the behavior of signals being raised while their
	   signal handler is being executed is historically ill-defined. To
	   make sure we handle all exited processes, we use waitpid in a loop
	   with WNOHANG. */
	signal(SIGCHLD, sigchldhandler);
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void init(int argc, char **argv) {
	char *config_file = CONFIGFILE;
	int i;
#ifdef ENABLE_PIDFILE
	pid_t pid;
	FILE *pidfile;
#endif
#ifdef ENABLE_LOGGING
	int logfd;
#endif

	/* Catch segfaults */
	signal(SIGSEGV, sighandler);
	/* Exorcise zombies */
	signal(SIGCHLD, sigchldhandler);

	config = get_default_config();
	if(!config) {
		perror("get_default_config");
		exit(EXIT_FAILURE);
	}

#ifdef ENABLE_SSL
	/* Initialize SSL */
	config->ssl_ctx = ssl_init();
	if(!config->ssl_ctx) {
		ssl_perror("ssl_init");
		exit(EXIT_FAILURE);
	}
#endif

	/* Parse command line */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-c")) {
			config_file = argv[++i];
		} else {
			fprintf(stderr, "ERROR: Invalid argument: %s\n",
				argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	/* Load configuration from config_file, storing it into config. */
	if(!read_config_file(config_file, config)) {
		fprintf(stderr, 
			"ERROR: Error loading configuration from \"%s\"\n",
			config_file);
		if(errno) perror("read_config_file");
		exit(EXIT_FAILURE);
	}

	/* Make the configuration we just loaded the global configuration. */
	set_current_config(config);
 
	/* If config->webroot is set, chroot to it. */
 	if(config->webroot) {
 		if(chroot(config->webroot)) {
 			fprintf(stderr, "chroot to %s: %s\n",
 				config->webroot, strerror(errno));
 			exit(EXIT_FAILURE);
 		}
 
 		if(chdir("/")) {
 			perror("chdir to new root directory");
 			exit(EXIT_FAILURE);
 		}
 	}
 
	/* Change directory to config->webdir. */
 	if(chdir(config->webdir)) {
		fprintf(stderr, "chdir to %s: %s\n",
 			config->webdir, strerror(errno));
 		exit(EXIT_FAILURE);
 	}

	/* Set up the environment */
	clearenv();
	setenv("SERVER_SOFTWARE", SERVER_SOFTWARE, 1);
	setenv("DOCUMENT_ROOT", config->webdir, 1);

	if(config->port) {
		sock = tcp_listen(config->port);
		if(sock == -1) {
			perror("tcp_listen");
			exit(EXIT_FAILURE);
		}
	} else {
		sock = -1;
	}

#ifdef ENABLE_SSL
	if(config->ssl_port) {
		ssl_sock = tcp_listen(config->ssl_port);
		if(ssl_sock == -1) {
			perror("tcp_listen");
			exit(EXIT_FAILURE);
		}
	} else {
		ssl_sock = -1;
	}
#endif

#ifdef ENABLE_BACKGROUND
	/* Background */
	pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if(pid) exit(EXIT_SUCCESS);
#endif

#ifdef ENABLE_PIDFILE
	/* Write pidfile */
	if(config->pidfile) {
		pidfile = fopen(config->pidfile, "w");
		if(pidfile) {
			if(fprintf(pidfile, "%ld\n", (long) getpid()) < 0) {
				perror(config->pidfile);
				fputs("WARNING: Could not write pidfile\n",
					stderr);
			}
			fclose(pidfile);
		} else {
			perror(config->pidfile);
			fputs("WARNING: Could not open pidfile\n", stderr);
		}
	}
#endif

#ifndef DISABLE_SETUID
	/* Set user id and group id */
	if(config->gid) {
		if(setgid(config->gid) != 0) {
			fputs("ERROR: Could not set group id\n",
				stderr);
			perror("setgid");
			exit(EXIT_FAILURE);
		}
	}

	if(config->uid) {
		if(setuid(config->uid) != 0) {
			fputs("ERROR: Could not set user id\n",
				stderr);
			perror("setuid");
			exit(EXIT_FAILURE);
		}
	}

	if(!getuid()) fputs("WARNING: Running as r00t\n",
		stderr);
#endif /* !DISABLE_SETUID */

	close(0);
	close(1);
	close(2);
#ifdef ENABLE_BACKGROUND
	/* Detach */
	setsid();
#endif /* ENABLE_BACKGROUND */

#ifdef ENABLE_LOGGING
	/* Connect logfile to standard error */
	if(config->logfile) {
		logfd = fileno(config->logfile);
		if(logfd >= 0) dup2(logfd, 2);
	}
#endif
}
