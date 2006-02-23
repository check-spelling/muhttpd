#include "flags.h"
#include "socket.h"
#include "status.h"
#include "init.h"
#include "serve.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

SOCKET sock;
#ifdef ENABLE_SSL
SOCKET ssl_sock;
#endif

static void accept_from(SOCKET sock, int ssl) {
	SOCKET conn;
	struct sockaddr saddr;
	socklen_t salen;
	pid_t pid;

	conn = accept(sock, &saddr, &salen);

	/* Fork a child to handle the connection */
	pid = fork();
	if(pid < 0) {
		perror("fork");
	} else if(pid) {
#ifdef ENABLE_SSL
		if(ssl) serve_ssl(&saddr, salen);
		else serve(&saddr, salen);
#else
		serve(&saddr, salen);
#endif
		exit(EXIT_SUCCESS);
	}
	/* close the socket in the parent process */
	closesocket(conn);
}

int main(int argc, char **argv) {
	fd_set fds;

	init(argc, argv);

	while(1) {
		/* Wait for a connection */
		FD_ZERO(&fds);
		if(sock != -1) FD_SET(sock, &fds);
#ifdef ENABLE_SSL
		if(ssl_sock != -1) FD_SET(ssl_sock, &fds);
		if(!FD_ISSET(ssl_sock, &fds)) {
			fprintf(stderr, "0 != 0\n");
			exit(EXIT_FAILURE);
		}
		if(select((sock < ssl_sock ? ssl_sock : sock) + 1,
			&fds, NULL, NULL, NULL) < 0) {
#else
		if(select(sock + 1, &fds, NULL, NULL, NULL) < 0) {
#endif
			if(errno == EINTR) continue;
			perror("select");
			exit(EXIT_FAILURE);
		}

		/* Accept a connection */
		if(FD_ISSET(sock, &fds)) accept_from(sock, 0);
#ifdef ENABLE_SSL
		if(FD_ISSET(ssl_sock, &fds)) accept_from(ssl_sock, 1);
#endif
	}

	/*@notreached@*/

	closesocket(sock);

	return 0;
}
