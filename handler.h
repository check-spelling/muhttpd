#ifndef HANDLER_H
#define HANDLER_H
#include "request.h"

#if defined(ENABLE_HANDLERS) || defined(ENABLE_CGI)
void invoke_handler(const char *handler, struct request *req);
#endif /* ENABLE_HANDLERS || ENABLE_CGI */

#ifdef ENABLE_CGI
void run_cgi(struct request *req);
#endif

#endif /* HANDLER_H */
