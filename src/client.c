#define _GNU_SOURCE
#include <sys/un.h>
#include <netinet/in.h>
#include <signal.h>
#include <ucontext.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> 
#include <assert.h>
#include <poll.h>

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

// from	linux/arch/x86/mm/fault.c
enum x86_pf_error_code {

        PF_PROT         =               1 << 0,
        PF_WRITE        =               1 << 1,
        PF_USER         =               1 << 2,
        PF_RSVD         =               1 << 3,
	PF_INSTR        =               1 << 4,
};


struct socket s;

void fault_handler(int sig, siginfo_t *info, void *ucontext) 
{
	int si_errno = info->si_errno;
	int si_signo = info->si_signo;
	void* addr = info->si_addr;

	ucontext_t *ctx = (ucontext_t*)ucontext;
	int err_type = ctx->uc_mcontext.gregs[REG_ERR];

	assert(si_signo == SIGSEGV && "expected a segfault");

	if (err_type & PF_WRITE) {
		//Write error
		// cstore(s.fd, addr);
		printf("write fault\n");
		exit(1);
	} else {
		//Read error
		// cload(s.fd, addr);
		printf("read fault\n");
		exit(1);
	}
}

/* do not use the following two, current bodge */
int request_socket;
int clients[5];

int main(int argc, char *argv[])
{
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
			fgets(buf, sizeof(buf), stdin);
			switch (buf[0]) {
			case 'w':
				
				break;
			case 'r':

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

