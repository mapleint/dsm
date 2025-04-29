#include <sys/un.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> 
#include <assert.h>

//#include "moesi.h"
//#include "packet.h"
#include "config.h"
#include "rpc.h"

int main(int argc, char *argv[])
{

	struct socket s = create_un(DEFAULT_SERVER_FILE);
	if (connects(&s)) {
		perror("connect");
		return 1;
	}
	printf("connection successful\n");

	char buf[25];
	while (scanf("%s", buf), buf[0] != 'q') {
		struct ping_args args = { 0 };
		struct ping_args resp = { 0 };
		strcpy(args.str, "PING");
		remote(s.fd, RPC_ping, &args, &resp);
		printf("rpc returned %s\n", resp.str);
	}

	printf("client exiting\n");
	close(s.fd);
}

