#include <stdlib.h>
#include <stdio.h>
#include <sys/un.h>

#include "moesi.h"
#include "packet.h"

int main(int argc, char *argv[])
{
	char *server_addr = getenv("SERVER");

	if (argc >= 2 && !server_addr) {
		server_addr = argv[1];
	}

	if (!server_addr) {
		fprintf(stderr, "SERVER not set\n");
		exit(1);
	}

	printf("connecting to server %s\n", server_addr);
}

