#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <pthread.h>

#include "config.h"
#include "rpc.h"

struct pollfd fds[NUM_CLIENTS] = { 0 };

extern __thread int request_socket;
extern int clients[NUM_CLIENTS];

void *run_main(__attribute((unused)) void *args)
{
	int return_code = -1;
	remote(clients[0], RPC_exec, NULL, &return_code);
	printf("'exitied' with code %d\n", return_code);
	// exit(return_code);
	return NULL;
}

int main(int argc, char *argv[])
{
	for (int i = 0; i < NUM_CLIENTS; i++) {
		clients[i] = -1;
	}

	struct socket s; 
	if (argc < 2) {
		printf("defaulting to UNIX\n");
		unlink(DEFAULT_SERVER_FILE);
		s = create_un(DEFAULT_SERVER_FILE);
	} else {
		s = create_in(NULL, atoi(argv[1]));
	}

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
	while (connected < NUM_CLIENTS) {
		int client_fd = accepts(&s);
		if (client_fd == -1) {
			perror("accept");
		}
		printf("accepted connection fd = %d\n", client_fd);
		int i = connected;
		clients[i] = client_fd;
		fds[i].fd = client_fd;
		fds[i].events = POLLIN;
		connected++;
	}
	printf("clients have connected, waiting to execute\n");
	getchar();
	printf("executing server\n");
	pthread_t master;
	original_pthread_create(&master, NULL, run_main, NULL);
	
	while (true) {
		poll(fds, NUM_CLIENTS, -1);
		for (int i = 0; i < connected; i++) {
			if (!(fds[i].revents & POLLIN)) {
				continue;
			}
			request_socket = fds[i].fd;
			if (fds[i].revents & POLLHUP) {
				printf("client %d hungup\n", i);
				connected--;
				close(fds[i].fd);
				continue;
			}
			printf("handling rpc from client #%d=%d\n", i, fds[i].fd);
			handle_s(fds[i].fd);
		}
	}
	return 0;
}

