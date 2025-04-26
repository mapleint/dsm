#include <sys/socket.h>
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
	binds(&s);
	printf("socket bound\n");
	listens(&s);
	printf("listening on socket\n");
	accepts(&s);
	printf("accepted connection\n");

	return 0;
}
