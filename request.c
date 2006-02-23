#include "flags.h"
#include "request.h"
#if defined(ENABLE_HANDLERS) || defined(ENABLE_CGI)
#include "handler.h"
#endif
#ifdef ENABLE_LOGGING
#include "log.h"
#endif
#include "status.h"
#include "globals.h"
#include "type.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#define ADDR_BYTE(X, N) \
	((char*) &(((struct sockaddr_in*) &(X))->sin_addr.s_addr))[N]

#define BUFSIZE 4096

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#define new(T) (T*) malloc(sizeof(T))

/** Read request */
static int read_request(char *buf, size_t len) {
	int n, m = 0;
	struct timeval tv;
	fd_set fds;

	/* Loop until full request received */
	for(;;) {
		/* Wait for data */
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		n = select(1, &fds, NULL, NULL, &tv);
		if(n < 0) {
			/* Interrupted system call */
			if(errno == EINTR) continue;
		} else if(n == 0) {
			/* Timeout */
			return HTTP_408;
		}
		
		/* Read data */
		n = read(0, &buf[m], 1);
		if(n <= 0) {
			if(errno == EAGAIN) continue;
			else return HTTP_400;
		}
		buf[++m] = 0; /* NUL-terminate request */

		/* If we got an empty line, we're done */
		if(!strcmp(&buf[m - 4], "\r\n\r\n") ||
			!strcmp(&buf[m - 2], "\n\n")) break;

	} /* loop until full request received */
	return 0;
}

/** Decode URL to filename */
static char *decode_url(const char *url, char *filename, size_t len) {
	char *p = (char*) url, *q = filename, *r = filename + len;

	while(*p) {
		if(*p == '+') *q = ' ';
		else if(*p == '?') break;
		else if(*p == '%') {
			p++;
			if('0' <= *p && *p <= '9') *q = *p - '0';
			else *q = (*p & ~32) - '7';
			*q = *q << 4;
			p++;
			if('0' <= *p && *p <= '9') *q |= *p - '0';
			else *q |= (*p & ~32) - '7';
		}
		else *q = *p;
		q++;
		p++;
		if(q == r && *p) {
			errno = ENAMETOOLONG;
			return NULL;
		}
	}

	*q = 0;
	return filename;
}

void do_request(struct sockaddr *addr, socklen_t salen) {
	char buf[BUFSIZE], *p;
	struct request req;
	int n;

	memset(&req, 0, sizeof(req));

	/* Set remote address */
	memcpy(&(req.remote_addr), addr, salen);

	n = read_request(buf, BUFSIZE);
	if(n) {
		/* Error reading request */
		if(n != HTTP_408) perror("read_request");
		else fputs("Timeout while waiting for request\n", stderr);
		req.filename = message_file[n];
		req.uri = message[n];
		req.status = n;
		handle_and_log_request(&req);
		return;
	}

	/* Set request method */
	p = strchr(buf, ' ');
	if(!p) {
		req.filename = message_file[HTTP_400];
		req.status = HTTP_400;
	}
	*p = 0;
	req.method = buf;

	/* See if method is one of the understood types */
	if(strcmp(req.method, "GET")
		&& strcmp(req.method, "POST")) {

		/* Method not implemented */
		req.filename = message_file[HTTP_501];
		req.status = HTTP_501;
	}

	/* Set request URI */
	req.uri = p + 1;
	p = strpbrk(req.uri, " \r\n");

	/* Detect protocol version */
	if(*p != ' ') req.proto = "HTTP/0.9";
	else {
		*p = 0;
		req.proto = p + 1;
		p = strpbrk(req.proto, "\r\n");
	}

	/* Zero *p, move p past end of line */
	p++;
	if((*p == '\r' || *p == '\n') && *p != *(p - 1)) {
		*(p - 1) = 0;
		p++;
	} else *(p - 1) = 0;

	/* Pass request buffer to helper functions */
	req.buf = p;

	/* Set status code */
	req.status = HTTP_200;

	/* Handle request */
	handle_and_log_request(&req);
}

void handle_request(struct request *req) {
	char filename[PATH_MAX], buf[BUFSIZE], *p;
	int fd, n, m;
	struct stat stats;

	/* If status != 200 and no filename, send status message */
	if(req->status != HTTP_200 && !req->filename) {
		send_status_message(req);
		return;
	}

	/* If no filename set, decode URL into filename */
	if(!req->filename) {
		if(!decode_url(req->uri, filename, PATH_MAX)) {
			req->filename = message_file[HTTP_414];
			req->status = HTTP_414;
			handle_request(req);
			return;
		}
		req->filename = filename;
	}

	/* Use ./ for root directory */
	if(!req->filename[1]) req->filename = "/./";

	/* stat file */
	if(stat(&req->filename[1], &stats) < 0) {
		if(req->status != HTTP_200) {
			/* Unable to stat error document, use message instead */
			send_status_message(req);
			return;
		} else if(errno == EACCES) {
			req->filename = message_file[HTTP_403];
			req->status = HTTP_403;
			handle_request(req);
			return;
		} else if(errno == ENOENT) {
			req->filename = message_file[HTTP_404];
			req->status = HTTP_404;
			handle_request(req);
			return;
		} else {
			req->filename = message_file[HTTP_500];
			req->status = HTTP_500;
			handle_request(req);
			return;
		}
	}

	/* Special case for directories */
	if(S_ISDIR(stats.st_mode)) {
		/* If filename doesn't end in '/', redirect client */
		if(req->filename[strlen(req->filename) - 1] != '/') {
			req->location = malloc(strlen(req->uri) + 2);
			if(!req->location) {
				req->status = HTTP_500;
				req->filename = message_file[HTTP_500];
				handle_request(req);
				return;
			}
			p = strchr(req->uri, '?');
			if(p) {
				strncpy(req->location, req->uri,
					(size_t) (p - req->uri));
				req->location[p - req->uri] = '/';
				strcpy(&req->location[p - req->uri + 1], p);
			} else {
				sprintf(req->location, "%s/", req->uri);
			}
			req->status = HTTP_301;
			req->filename = message_file[HTTP_301];
			handle_request(req);
			return;
		} else {
			/* Send index file */
			for(n = 0; n < config->indices; n++) {
				p = malloc(strlen(req->filename)
					+ strlen(config->index[n]) + 1);
				if(!p) {
					req->status = HTTP_500;
					req->filename = message_file[HTTP_500];
					handle_request(req);
					return;
				}
				sprintf(p, "%s%s", req->filename,
					config->index[n]);

				if(!stat(&p[1], &stats)) {
					req->filename = p;
					handle_request(req);
					return;
				}
				free(p);
			}
		}
	}

	if(!S_ISREG(stats.st_mode)) {
		/* Refuse to serve special files */
		req->filename = message_file[HTTP_403];
		req->status = HTTP_403;
		handle_request(req);
		return;
	}

#ifndef DISABLE_MIME
	/* Get type */
	req->file_type = get_file_type(req->filename);

	if(req->file_type) {
#ifdef ENABLE_HANDLERS
		/* If the file type has a handler, invoke it */
		if(req->file_type->handler) {
			invoke_handler(req->file_type->handler, req);
			return;
		}
#endif /* ENABLE_HANDLERS */
	}
#endif /* DISABLE_MIME */

#ifdef ENABLE_CGI
	/* If file is executable, run it */
	if(stats.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
		run_cgi(req);
		return;
	}
#endif /* ENABLE_CGI */

	/* Open file */
	fd = open(&req->filename[1], O_RDONLY);
	if(fd < 0) {
		/* open failed, inform client */
		if(req->status != HTTP_200) {
			/* Unable to send error document; send error message */
			send_status_message(req);
			return;
		} else if(errno == EACCES) {
			req->filename = message_file[HTTP_403];
			req->status = HTTP_403;
			handle_request(req);
			return;
		} else {
			req->filename = message_file[HTTP_500];
			req->status = HTTP_500;
			handle_request(req);
			return;
		}
	}

	/* Emit headers */
	if(req->file_type)
		printf("HTTP/1.1 %s\r\nConnection: close\r\nContent-Type: %s"
		"\r\n\r\n", message[req->status], req->file_type->mime_name);
	else printf("HTTP/1.1 %s\r\nConnection: close\r\n\r\n",
		message[req->status]);
	fflush(stdout);

	/* Beam the data over */
	while((n = (int) read(fd, buf, BUFSIZE)) > 0) {
		p = buf;
		do {
			m = write(1, p, n);
			if(m <= 0) break;
			p += m;
			n -= m;
		} while(n);
		if(m <= 0) break;
	}
}

void handle_and_log_request(struct request *req) {
	handle_request(req);

#ifdef ENABLE_LOGGING
	if(current_config->logfile) log_request(req);
#endif
}
