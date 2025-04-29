#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>

#include "config.h"
#include "rpc.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("defaulting to UNIX\n");
		unlink(DEFAULT_SERVER_FILE);
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

	struct pollfd fds[1] = { 0 };

	fds[0].events = POLLIN;
	fds[0].fd = cfd;
	while (true) {
		printf("polling\n");
		poll(fds, 1, -1);
		printf("poll returned\n");
		for (int i = 0; i < sizeof(fds) / sizeof(*fds); i++) {
			if (fds[i].revents & POLLIN) {
				printf("handling\n");
				remote_handler(fds[i].fd);
			}
		}
	}
	return 0;
}
