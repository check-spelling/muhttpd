#include "config.h"
#include "log.h"
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

void log_request(const struct request *req) {
	time_t t;
	struct tm *tm;

	time(&t);
	tm = gmtime(&t);

	fprintf(current_config->logfile,
		"%d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d\t"
		"%s\t%s %s\n",
		tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		inet_ntoa(req->remote_addr.sin_addr),
		req->method, req->uri);	
}
