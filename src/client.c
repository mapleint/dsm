#include "client.h"

void *userin(void *args)
{
	while (true) {
		char buf[25] = { 0 };
		fgets(buf, sizeof(buf), stdin);
		char *global = (char*)shmem;
		switch (buf[0]) {
		case 'w':
			printf("writing\n");
			strcpy(global, buf + 1);
			printf("wrote\n");
			break;
		case 'r':
			printf("reading global\n");
			strcpy(buf, global);
			printf("read %s\n", buf);
			break;
		case 'p':
			cping(s.fd);
			break;
		case 'q':
			printf("client exiting");
			exit(0);
		default:
			printf("bad command :(\n");
			break;
		}
	}
	return NULL;
}

int main()
{
	shmem_init();
	// Initializing the signal handler
	struct sigaction sa = {
		.sa_sigaction = fault_handler,
		.sa_flags = SA_SIGINFO,
	};
	sigaction(SIGSEGV, &sa, NULL);

	s = create_un(DEFAULT_SERVER_FILE);
	if (connects(&s)) {
		perror("connect");
		return 1;
	}
	printf("connection successful\n");

	struct pollfd fds[1] = { 0 };
	fds[0].fd = s.fd, fds[0].events = POLLIN;
	pthread_t thread;
	original_pthread_create(&thread, NULL, userin, NULL);
	while (true) {
		poll(fds, 1, -1);
		short socket_revents = fds[0].revents;
		if (socket_revents & POLLIN) {
			/* server packet */
			printf("server rpc detected\n");
			handle_s(s.fd);
		}
	}
	printf("client exiting\n");
	close(s.fd);
}

