#ifndef SERVE_H
#define SERVE_H

#include "flags.h"
#include <sys/types.h>
#include <sys/socket.h>

void serve(struct sockaddr *addr, socklen_t salen);
#ifdef ENABLE_SSL
void serve_ssl(struct sockaddr *addr, socklen_t salen);
#endif

#endif /* ndef SERVE_H */
