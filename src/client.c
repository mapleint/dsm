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
#include "memory.h"

struct page_entry {
	void *addr;
	enum state st;
};

#define NUM_ENTRIES 256

struct page_entry pe_cache[NUM_ENTRIES];

struct page_entry *insert_page(void *addr, enum state st)
{
	for (int i = 0; i < NUM_ENTRIES; i++) {
		if (pe_cache[i].st == INVALID || pe_cache[i].addr == addr) {
			pe_cache[i].addr = addr;
			pe_cache[i].st = st;
			return &pe_cache[i];
		}
	}
	return NULL;
}

struct page_entry *find_page(void *addr)
{
	for (int i = 0; i < NUM_ENTRIES; i++) {
		if (pe_cache[i].addr == addr) {
			return &pe_cache[i];
		}
	}
	return NULL;

}

bool delete_page(void *addr)
{
	struct page_entry *entry = find_page(addr);
	if (!entry->addr) {
		return false;
	}
	entry->st = INVALID;
	entry->addr = NULL;
	return true;
}

void cping(int s)
{
	struct ping_args args = { 0 };
	struct ping_resp resp = { 0 };
	strcpy(args.str, "PING");
	remote(s, RPC_ping, &args, &resp);
	printf("%s\n", resp.str);
}

void cload(int s, void *addr)
{
	addr = page_align(addr);
	struct load_args args = { .addr = addr };
	struct load_resp resp = { 0 };
	remote(s, RPC_load, &args, &resp);
	r_prot(addr);
	assert(insert_page(addr, resp.st)); // TODO flesh out this logic
}

void cstore(int s, void *addr)
{
	addr = page_align(addr);
	struct page_entry *pe = find_page(addr);
	if (pe && pe->st == EXCLUSIVE) {
		// if we have it exclusive, we can skip the RPC
		goto skip;
	} else if (!pe) {
		pe = insert_page(addr, INVALID);
	}
	assert(pe && "might need to make cache larger");

	struct store_args args = { .addr = addr };
	struct store_resp resp = { 0 };
	remote(s, RPC_store, &args, &resp);
skip:
	rw_prot(pe->addr);
	pe->st = MODIFIED;
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
	// int si_errno = info->si_errno;
	void* addr = info->si_addr;

	ucontext_t *ctx = (ucontext_t*)ucontext;
	int err_type = ctx->uc_mcontext.gregs[REG_ERR];

	assert(sig == SIGSEGV && "expected a segfault");

	if (err_type & PF_WRITE) {
		//Write error
		cstore(s.fd, addr);
	} else {
		//Read error
		cload(s.fd, addr);
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
			char *global = (char*)shmem;
			switch (buf[0]) {
			case 'w':
				printf("writing %d\n");
				strcpy(global, buf);
				break;
			case 'r':
				printf("reading global:\n%s\n", global);
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
		}
	}

	printf("client exiting\n");
	close(s.fd);
}

