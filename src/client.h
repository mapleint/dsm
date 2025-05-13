
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
#include <pthread.h>

#include "config.h"
#include "rpc.h"
#include "memory.h"

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
	rw_prot(addr);
	memcpy(addr, resp.page, PAGE_SIZE);
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
	printf("cstore addr %p\n", addr);

	rw_prot(addr);
	memcpy(addr, resp.page, PAGE_SIZE);
	pe->st = MODIFIED;
	printf("exiting cstore\n");
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
	printf("fault addr %p\n", addr);

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
