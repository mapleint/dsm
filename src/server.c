#include <sys/socket.h>
#include <stdboo.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("defaulting to IP\n");
	}

	printf("server started\n");
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);

	return 0;
}
