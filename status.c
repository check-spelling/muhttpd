#include "status.h"
#include <stdio.h>

/* Error messages */
/* NB: These MUST match the error codes defined in status.h */
char *message[] = {
	"200 OK",
	"301 Moved Permanently",
	"400 Bad Request",
	"403 Forbidden",
	"404 Not Found",
	"408 Request Timeout",
	"413 Request Entity Too Large",
	"414 Request-URI Too Long",
	"500 Internal Server Error",
	"501 Not Implemented"};

char *message_file[sizeof(message) / sizeof(char*)];

void send_status_message(struct request *req) {
	printf("HTTP/1.1 %s\r\nContent-Type: text/html\r\n"
		"Conection: close\r\n", message[req->status]);
	if(req->location) printf("Location: %s\r\n\r\n"
		"<html>\n<head>\n<title>%s</title>\n</head>\n"
		"<body>\n<h1>%s</h1>\n<p>Object has moved to <a href=\"%s\">"
		"%s</a></p>\n</body>\n</html>\n", req->location,
		message[req->status], message[req->status],
		req->location, req->location);
	else printf("\r\n<html>\n<head>\n"
		"<title>%s</title>\n</head>\n<body>\n<h1>%s</h1>\n"
		"</body>\n</html>\n", message[req->status], message[req->status]);
	fflush(stdout);
}
