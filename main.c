#include "flags.h"
#include "socket.h"
#include "status.h"
#include "init.h"
#include "request.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

SOCKET sock;

int main(int argc, char **argv) {
	SOCKET conn;

	init(argc, argv);

	while(1) {
		/* Accept a connection */
		conn = accept(sock, NULL, 0);
		/* Fork a child to handle the connection */
		if(!fork()) {
			/* Duplicate the socket on stdout */
			dup(conn);

			serve();
			exit(EXIT_SUCCESS);
		}
		/* close the socket in the parent process */
		closesocket(conn);
	}

	/*@notreached@*/

	closesocket(sock);

	return 0;
}
