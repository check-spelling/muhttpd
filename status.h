#ifndef STATUS_H
#define STATUS_H
#include "request.h"

void send_status_message(struct request *req);

/* Status codes */
#define HTTP_200 0
#define HTTP_301 1
#define HTTP_400 2
#define HTTP_403 3
#define HTTP_404 4
#define HTTP_408 5
#define HTTP_413 6
#define HTTP_414 7 
#define HTTP_500 8
#define HTTP_501 9

#endif /* STATUS_H */
