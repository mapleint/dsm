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

in_addr_t is_ipv4(char *str)
{
	assert("str must not be nullptr" && str);
	char *p = strdup(str);
	assert(p);
	char ip[4];
	int i = 0;
	char *tok = strtok(p, ".");
	for (i = 0; i < 4 && tok; i++) {
		ip[i] = atoi(tok);
		tok = strtok(NULL, ".");
	}
	free(p);
	return *(int*)ip;
}

int main(int argc, char *argv[])
{
	/*
	char *server_addr = getenv("SERVER");

	if (argc >= 2 && !server_addr) {
		server_addr = argv[1];
	}

	if (!server_addr) {
		fprintf(stderr, "SERVER not set\n");
		exit(1);
	}
	
	printf("connecting to server %s\n", server_addr);
	is_ipv4(server_addr);
	*/

	struct socket s = create_un(DEFAULT_SERVER_FILE);
	if (connects(&s)) {
		perror("connect");
		return 1;
	}
	printf("connection successful\n");
	int sen = write(s.fd, "ping", 5);
	printf("client wrote %d bytes\n", sen);
	char rb[10] = { 0 };
	int red = read(s.fd, rb, 5);
	printf("client received %d bytes:%s\n", red, rb);
	close(s.fd);
}

