#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>

#include "config.h"
#include "rpc.h"

struct pollfd fds[NUM_CLIENTS + 1] = { 0 };

extern int request_socket;
extern int clients[NUM_CLIENTS];

int main(int argc, char *argv[])
{
	for (int i = 0; i < NUM_CLIENTS; i++) {
		clients[i] = -1;
	}

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

	fds->fd = s.fd;
	fds->events = POLLIN;
	int connected = 0;
	while (true) {
		poll(fds, connected + 1, -1);
		if (fds->revents & POLLIN) {
			int j = ++connected;
			int client_fd = accepts(&s);
			if (client_fd == -1) {
				perror("accept");
			}
			printf("accepted connection %d\n", client_fd);
			fds[j].fd = client_fd;
			clients[j-1] = client_fd;
			fds[j].events = POLLIN;
		}
		for (int i = 1; i < connected + 1; i++) {
			if (!(fds[i].revents & POLLIN)) {
				continue;
			}
			request_socket = fds[i].fd;
			// TODO: remove client on POLLHUP
			printf("handling rpc from server %d\n", fds[i].fd);
			remote_handler(fds[i].fd);
		}
	}
	return 0;
}
