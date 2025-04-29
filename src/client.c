#include <sys/un.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> 
#include <assert.h>
#include <poll.h>

//#include "moesi.h"
//#include "packet.h"
#include "config.h"
#include "rpc.h"

int cping(int s)
{
	struct ping_args args = { 0 };
	struct ping_resp resp = { 0 };
	strcpy(args.str, "PING");
	remote(s, RPC_ping, &args, &resp);
	printf("%s\n", resp.str);
}

extern enum state mesi_st;
int cload(int s, void *addr)
{
	if (mesi_st == MODIFIED) return;
	struct load_args args = { .addr = addr };
	struct load_resp resp = { 0 };
	remote(s, RPC_load, &args, &resp);
	mesi_st = resp.st;

}

int cstore(int s, void *addr)
{
	if (mesi_st == MODIFIED) return;
	if (mesi_st == EXCLUSIVE) {
		// mark as write
		mesi_st = MODIFIED;
		return;
	}
	struct store_args args = { .addr = addr };
	struct store_resp resp = { 0 };
	remote(s, RPC_store, &args, &resp);
	mesi_st = resp.st;
}

/* do not use the following two, current bodge */
int request_socket;
int clients[5];

int main(int argc, char *argv[])
{

	struct socket s = create_un(DEFAULT_SERVER_FILE);
	if (connects(&s)) {
		perror("connect");
		return 1;
	}
	printf("connection successful\n");

	struct pollfd fds[2] = { 0 };

	fds[0].fd = s.fd, fds[0].events = POLLIN;
	fds[1].fd = 0, fds[1].events = POLLIN;
	while (true) {
		char buf[25] = { 0 };
		poll(fds, 2, -1);
		short socket_revents = fds[0].revents;
		if (socket_revents & POLLIN) {
			/* SERVER TOLD US SOMETHING UNPROMPTED? */
			printf("server rpc detected\n");
			remote_handler(s.fd);
		}
		short stdin_revents = fds[1].revents;
		if (stdin_revents & POLLIN) {
			scanf("%s", buf);
			switch (buf[0]) {
			case 'w':
				cstore(s.fd, 0);
				break;
			case 'r':
				cload(s.fd, 0);
				break;
			case 'p':
				cping(s.fd);
				break;
			case 'q':
				exit(0);
			default:
				printf("bad command :(\n");
				break;
			}
			printf("mesi %d\n", mesi_st);
		}
	}

	printf("client exiting\n");
	close(s.fd);
}

