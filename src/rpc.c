#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "rpc.h"
#include "config.h"
#include "memory.h"
#include <signal.h>
#include <pthread.h>

/* TODO:
 *
 * THIS IS THE DUMBEST BODGE I'VE PROGRAMMED
 */

/* do not use the following two, current bodge */
int request_socket;
int clients[NUM_CLIENTS];

extern struct page_entry pe_cache[NUM_ENTRIES];

static socklen_t socklen(struct socket* sock) {
	switch (sock->in.sin_family) {
	case AF_INET:
		return sizeof(sock->in);
		break;
	case AF_UNIX:
		return sizeof(sock->un);
		break;
	default:
		fprintf(stderr, "bruh wtf???");
		break;
	}
	return -1;
}


struct socket create_in(const char *idk)
{

}

struct socket create_un(const char *path)
{
	struct socket s = { 0 };
	s.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s.fd < 0) {
		fprintf(stderr, "socket creation failed\n");
		exit(1);
	}
	s.un.sun_family = AF_UNIX;
	s.len = socklen(&s);
	memcpy(s.un.sun_path, path, strlen(path));
	return s;
}

int accepts(struct socket *s)
{
	return accept(s->fd, (struct sockaddr *) &s->un, &s->len);
}

int listens(struct socket *s)
{
	return listen(s->fd, 1);
}

int binds(struct socket *s)
{
	return bind(s->fd, (struct sockaddr *) &s->un, socklen(s));
}

int connects(struct socket *s)
{
	return connect(s->fd, (struct sockaddr *) &s->un, socklen(s));
}

int sends(int con, void *buf, unsigned len)
{
	unsigned sent = 0;
	while (sent < len) {
		int trans = write(con, (char*)buf + sent, len - sent);
		if (trans < 0) {
			perror("write");
			return 0;
		}
		sent += trans;
	}
	return 1;
}

int recvs(int con, void *buf, unsigned len)
{
	unsigned recvd = 0;
	while (recvd < len) {
		int trans = read(con, (char*)buf + recvd, len - recvd);
		if (trans < 0) {
			perror("read");
			return 0;
		}
		recvd += trans;
	}
	return 1;
}


void ping(void *pparams, void *presult)
{
	struct ping_args *params = pparams;
	struct ping_resp *result = presult;

	if (strcmp(params->str, "PING") == 0) {
		strcpy(result->str, "PONG");
	} else {
		strcpy(result->str, "FAIL");
	}

}
/* probe_(.*) rpcs should only be ran on clients */
void probe_read(void* pparams, void* presult)
{
	struct pr_args *args = pparams;
	struct pr_resp *result = presult;
	// TODO lookup by addr
	struct page_entry *pe = find_page(args->addr);
	enum state st = pe ? pe->st : INVALID;
	result->st = st;
	printf("pre probe %d\n", st);
	// query page desc 
	switch (st) {
	case MODIFIED:
	case EXCLUSIVE:
		st = SHARED;
		r_prot(args->addr);
	case SHARED:
		memcpy(&result->page, args->addr, PAGE_SIZE);
	case INVALID:
		break;
	}
	printf("post probe %d\n", st);
	
	// if (invalid) return;
	// if (modified) return mark_shared, page;
	// if (shared) return mark_shared, parge;
	// if (exclusive) return;

}

void probe_write(void* pparams, void* presult)
{
	struct pw_args *args = pparams;
	struct pw_resp *result = presult;
	struct page_entry *pe = find_page(args->addr);
	enum state st = pe ? pe->st : INVALID;
	result->st = st;
	switch (st) {
	case SHARED:
	case MODIFIED:
	case EXCLUSIVE:
		memcpy(result->page, args->addr, PAGE_SIZE);
		break;
	case INVALID:
		break;
	}
	st = INVALID;
	no_prot(args->addr);
	printf("post probe %d\n", st);
	// query page desc
	// if (invalid) return;
	// if (modified) return mark_inavlid, page;
	// if (shared) return mark_invalid;
	// if (exclusive) return mark_invalid;
}

/* load/store rpcs should only be ran on server */
void load(void* pparams, void* presult)
{
	struct load_args *params = pparams;
	struct load_resp *result = presult;

	printf("load handler running\n");
	struct pr_args args = { 0 };
	struct pr_resp resp = { 0 };
	args.addr = params->addr;
	result->st = EXCLUSIVE;
	for (int i = 0; i < NUM_CLIENTS; i++) {
		int client_fd = clients[i];
		if (client_fd == request_socket || client_fd < 0) {
			continue;
		}
		printf("probing`%d\n", client_fd);
		remote(client_fd, RPC_probe_read, &args, &resp);
		printf("response from`%d\n", client_fd);
		if (resp.st == INVALID) {
			continue;
		}
		printf("pg data %s\n", &resp.page);
		memcpy(result->page, &resp.page, sizeof(resp.page));
		result->st = SHARED;
	}

}

void store(void* pparams, void* presult)
{
	struct store_args *params = pparams;
	struct store_resp *result = presult;

	printf("store handler running\n");
	struct pw_args args = { 0 };
	struct pw_resp resp = { 0 };
	args.addr = params->addr;
	result->st = MODIFIED;
	for (int i = 0; i < NUM_CLIENTS; i++) {
		int client_fd = clients[i];
		if (client_fd == request_socket || client_fd < 0) {
			continue;
		}
		printf("probing`%d\n", client_fd);
		remote(client_fd, RPC_probe_write, &args, &resp);
		printf("response from`%d\n", client_fd);
		if (resp.st == INVALID) {
			continue;
		}
		memcpy(result->page, &resp.page, sizeof(resp.page));
	}

}

struct rpc_inf rpc_inf_table[] = {
	[RPC_NA] = {NULL, -1, -1},
	[RPC_ping] = {ping,
	       	sizeof(struct ping_args), sizeof(struct ping_resp)},

	[RPC_probe_read] = {probe_read,
	       	sizeof(struct pr_args), sizeof(struct pr_resp)},
	[RPC_probe_write] = {probe_write,
	       	sizeof(struct pw_args), sizeof(struct pw_resp)},

	[RPC_load] = {load,
	       	sizeof(struct load_args), sizeof(struct load_resp)},
	[RPC_store] = {store,
	       	sizeof(struct store_args), sizeof(struct store_resp)},

};

int remote(int target, int func, void *input, void *output)
{
	struct rpc_inf *inf = rpc_inf_table + func;
	sends(target, &func, sizeof(int));
	sends(target, input, inf->param_sz);
	recvs(target, output, inf->response_sz);
	return 0;
}

void remote_handler(int caller)
{
	int func;

	recvs(caller, &func, sizeof(int));
	if (func < 0 || func >= RPC_MAX) {
		close(caller);
		return;
	}
	struct rpc_inf *inf = rpc_inf_table + func;

	void *resp = malloc(inf->response_sz);
	if (!resp) {
		perror("malloc");
		exit(EXIT_FAILURE);
		return;
	}
	void *args = malloc(inf->param_sz);
	if (!args) {
		perror("malloc");
		exit(EXIT_FAILURE);
		return;
	}

	recvs(caller, args, inf->param_sz);

	rpc_inf_table[func].handler(args, resp);
	free(args);
	sends(caller, resp, inf->response_sz);
	free(resp);
}

void sched(void* sched_args, void* sched_resp)
{
	// finds a slave to run procedure
	// currently just a simple round robin approach
    static int i = 0;
    while (clients[i] != request_socket) {
	    i = (i + 1) % NUM_CLIENTS;
    }

    remote(clients[i], RPC_run, sched_args, sched_resp);
	// remote(i, RPC_run, args, resp);

}

void run(void* run_args, void* run_resp)
{
    pthread_t thread;


    struct sched_args* sched_args = (struct sched_args*) run_args;

    struct spawn_args {
        int m1;
        int n1;
        int m2;
        int n2;
        char *addr1;
        char *addr2;
        char *dest;
    };
    struct spawn_args args = {sched_args->m1, sched_args->n1, sched_args->m2, sched_args->n2, sched_args->addr1, sched_args->addr2, sched_args->dest};
    pthread_create(&thread, NULL, (void*)(sched_args->func), &args);

}

void wait(void* /*struct wait_args*/, void* /*struct wait_resp*/)
{

}

