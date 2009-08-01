#include "serve.h"
#include "request.h"
#include <unistd.h>
#ifdef ENABLE_SSL
#include "config.h"
#include "ssl.h"
#include <openssl/ssl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef ENABLE_SSL
#define CLOSE_AND_EXIT(X) \
	close(stdins[1]); \
	close(stdouts[0]); \
	SSL_shutdown(ssl); \
	SSL_free(ssl); \
	exit(X)

static ssize_t write_all(int fd, char *buf, size_t len) {
	ssize_t i = 0, n;

	while(((size_t) i) < len) {
		n = write(fd, &buf[i], len - i);
		if(n < 0) return n;
		else if(n == 0) break;
		else i += n;
	}
	return i;
}
#endif

void serve(struct sockaddr *addr, socklen_t salen) {
	/* dup connection on stdout */
        if(dup(0) < 0) {
	  perror("dup");
	  exit(EXIT_FAILURE);
	}
	do_request(addr, salen);
}

#ifdef ENABLE_SSL
void serve_ssl(struct sockaddr *addr, socklen_t salen) {
	SSL *ssl;
	pid_t pid;
	int stdins[2];
	int stdouts[2];
	fd_set fds;
	ssize_t n, m;
	char buffer[16384];

	if(pipe(stdins) < 0) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if(pipe(stdouts) < 0) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	/* Do SSL handshake */
	ssl = ssl_accept_client(0, current_config->ssl_ctx);
	if(!ssl) {
		ssl_perror("ssl_accept_client");
		exit(EXIT_FAILURE);
	}

	/* Set HTTPS environment variable */
	putenv("HTTPS=true");

	pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if(pid) {
		/* close read end of stdins and write end of stdouts */
		close(stdins[0]);
		close(stdouts[1]);
	} else {
		/* close write end of stdins and read end of stdouts */
		close(stdins[1]);
		close(stdouts[0]);

		/* move read end of stdins to 0 */
		close(0);
		if(dup(stdins[0]) < 0) {
		  perror("dup");
		  exit(EXIT_FAILURE);
		}
		close(stdins[0]);

		/* move write end of stdouts to 1 */
		close(1);
		if(dup(stdouts[1]) < 0) {
		  perror("dup");
		  exit(EXIT_FAILURE);
		}
		close(stdouts[1]);

		/* handle request */
		do_request(addr, salen);
		exit(EXIT_SUCCESS);
	}

	/* Pass data between SSL socket and handling process */
	while(1) {
		FD_ZERO(&fds);
		if(stdins[1] != -1) FD_SET(0, &fds);
		FD_SET(stdouts[0], &fds);

		if(select(stdouts[0] + 1, &fds, NULL, NULL, NULL) < 0) {
			if(errno == EINTR) continue;
			perror("select");
			exit(EXIT_FAILURE);
		}

		/* Pump data from client to handler */
		if(FD_ISSET(0, &fds)) {
			n = (ssize_t) SSL_read(ssl, buffer,
				(int) sizeof(buffer));
			if(n == 0) {
				close(stdins[1]);
				stdins[1] = -1;
			} else if(n < 0) {
				ssl_perror("SSL_read");
				CLOSE_AND_EXIT(EXIT_FAILURE);
			} else {
				m = write_all(stdins[1], buffer, (size_t) n);
				if(m < 0) {
					perror("write_all");
					CLOSE_AND_EXIT(EXIT_FAILURE);
				}
			}
		}

		/* Pump data from handler to client */
		if(FD_ISSET(stdouts[0], &fds)) {
			n = read(stdouts[0], buffer, sizeof(buffer));
			if(n == 0) {
				CLOSE_AND_EXIT(EXIT_SUCCESS);
			} else if(n < 0) {
				perror("read");
				CLOSE_AND_EXIT(EXIT_FAILURE);
			} else {
				m = (ssize_t) SSL_write(ssl, buffer, (int) n);
				if(m < 0) {
					ssl_perror("SSL_write");
					CLOSE_AND_EXIT(EXIT_FAILURE);
				}
			}
		}
	}
}
#endif
