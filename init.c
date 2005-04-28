#include "flags.h"
#include "socket.h"
#include "type.h"
#include "config.h"
#include "memory.h"
#include "globals.h"
#include "clearenv.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef CONFIGFILE
#define CONFIGFILE "/etc/muhttpd/muhtppd.conf"
#endif

extern SOCKET sock;	/* from main.c */

static void sighandler(int num) {
	fprintf(stderr, "Caught signal %d\n", num);
	exit(0xff);
}

void init(int argc, char **argv) {
	char *config_file = CONFIGFILE;
	int i;
#ifdef ENABLE_PIDFILE
	pid_t pid;
	FILE *pidfile;
#endif

	/* Catch segfaults */
	signal(SIGSEGV, sighandler);
	/* Exorcise zombies */
	signal(SIGCHLD, SIG_IGN);

	config = get_default_config();
	if(!config) {
		perror("get_default_config");
		exit(1);
	}

	/* Parse command line */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-c")) {
			config_file = argv[++i];
		} else {
			fprintf(stderr, "ERROR: Invalid argument: %s\n",
				argv[i]);
			exit(0x80);
		}
	}

	if(!read_config_file(config_file, config)) {
		fprintf(stderr, 
			"ERROR: Error loading configuration from \"%s\"\n",
			config_file);
		if(errno) perror("read_config_file");
		exit(1);
	}

	set_current_config(config);
	chdir(config->webdir);

	/* Set up the environment */
	clearenv();
	setenv("SERVER_SOFTWARE", "muhttpd/0.10", 1);
	setenv("DOCUMENT_ROOT", config->webdir, 1);

	sock = tcp_listen(config->port);
	if(sock == -1) {
		perror("tcp_listen");
		exit(1);
	}

#ifdef ENABLE_BACKGROUND
	/* Background */
	pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(1);
	} else if(pid) exit;
#endif

#ifdef ENABLE_PIDFILE
	/* Write pidfile */
	pidfile = fopen(config->pidfile, "w");
	if(fprintf(pidfile, "%d\n", getpid()) < 0) {
		perror(config->pidfile);
		fputs("WARNING: Could not write pidfile\n", stderr);
	}
	fclose(pidfile);
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

	/* Detach */
	close(0);
	close(1);
	close(2);
	setsid();
}
