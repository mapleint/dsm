#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "rpc.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("defaulting to UNIX\n");
	}

	struct socket s = create_un(DEFAULT_SERVER_FILE);
	printf("socket created\n");
	if (binds(&s)) {
		perror("bind");
	}
	printf("socket bound\n");
	if (listens(&s)) {
		perror("listen");
	}
	printf("listening on socket\n");
	int cfd = accepts(&s);
	if (cfd == -1) {
		perror("accept");
	}
	printf("accepted connection\n");
	char rb[10] = { 0 };
	int red = read(cfd, rb, 5);
	if (red < 0) {
		perror("read");
	}
	printf("server received %d bytes: %s\n", red, rb);
	write(cfd, "pong", 5);
	printf("server wrote pong\n");
	return 0;
}
