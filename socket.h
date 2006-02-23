#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <unistd.h>

#define SOCKET int
#define closesocket close
#define INVALID_SOCKET -1

SOCKET tcp_listen(in_port_t port);

#endif /* SOCKET_H */
