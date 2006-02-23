#include "flags.h"
#include "socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

SOCKET tcp_listen(short port) {
	SOCKET sock;
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) return -1;
	if(bind(sock, (struct sockaddr*) &addr,
		(socklen_t) sizeof(addr)) < 0) {

		closesocket(sock);
		return -1;
	}
	
	if(listen(sock, 3) < 0) {
		closesocket(sock);
		return -1;
	}

	return sock;
}

