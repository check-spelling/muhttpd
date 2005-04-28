#include "flags.h"
#include "socket.h"
#include "status.h"
#include "init.h"
#include "request.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

SOCKET sock;

int main(int argc, char **argv) {
	SOCKET conn;

	init(argc, argv);

	while(1) {
		/* Accept a connection */
		conn = accept(sock, NULL, 0);
		/* Fork a child to handle the connection */
		if(!fork()) {
			/* Attach the socket to stdin and stdout */
			close(0);
			close(1);
			dup(conn);
			dup(conn);
			closesocket(conn);

			serve();
			exit(0);
		}
		/* close the socket in the parent process */
		closesocket(conn);
	}

	/*@notreached@*/

	closesocket(sock);

	return 0;
}
