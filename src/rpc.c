#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>
       
#include "rpc.h"
#include "config.h"
#include "memory.h"

__thread int request_socket;
int clients[NUM_CLIENTS];

extern struct page_entry pe_cache[NUM_ENTRIES];

struct rpc_wake {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int id;
	bool responded;
	void *buffer; // to hold response
	int len; // requested amount to read from buffer
};

struct wait_queue {
	struct rpc_wake *rec;
	struct wait_queue *next;
};

static struct wait_queue *outstanding_rpcs;

static void insert_rpc(struct rpc_wake *rec)
{
	struct wait_queue *link = malloc(sizeof(struct wait_queue));
	link->rec = rec;
	link->next = outstanding_rpcs;
	outstanding_rpcs = link;
}

static void unlink_rpc(struct wait_queue *prev, struct wait_queue *to_remove)
{
	assert(to_remove && "must not be null");
	if (!prev) {
		outstanding_rpcs = to_remove->next;
	} else {
		prev->next = to_remove->next;
	}
}

static struct rpc_wake *pop_rpc(int id)
{
	struct wait_queue *cur = outstanding_rpcs;
	struct wait_queue *prev = NULL;
	while (cur) {
		if (id == cur->rec->id) {
			unlink_rpc(prev, cur);
			struct rpc_wake *rec = cur->rec;
			free(cur);
			return rec;
		}

		prev = cur;
		cur = cur->next;
	}
	return NULL;

}

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


struct socket create_in(const char *ip, unsigned short port)
{
	struct socket s = { 0 };
	s.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (s.fd < 0) {
		fprintf(stderr, "in socket creation failed\n");
		exit(1);
	}
	s.in.sin_family = AF_INET;
	s.in.sin_port = htons(port);
	if (!ip) {
		s.in.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (inet_pton(AF_INET, ip, &s.in.sin_addr) <= 0) {
			perror("inet_pton");
			exit(EXIT_FAILURE);
		}
	}

	s.len = socklen(&s);

	return s;
}

struct socket create_un(const char *path)
{
	struct socket s = { 0 };
	s.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s.fd < 0) {
		fprintf(stderr, "un socket creation failed\n");
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
	// lookup mesi state by addr
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
		printf("pg data %s\n", (char*)&resp.page);
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
	[RPC_resp] = {NULL, -1, -1},
	[RPC_notif] = {NULL, sizeof(int), -1},
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

	[RPC_run] = {run,
	       	sizeof(struct thread_args), 0},

	[RPC_sched] = {sched,
	       	sizeof(struct thread_args), 0},

	[RPC_exec] = {exec_main,
	       	0, sizeof(int)},

};

int remote(int target, int func, void *input, void *output)
{
	static _Atomic int rpc_nonce = 0;
	struct rpc_inf *inf = rpc_inf_table + func;
	rpc_nonce++;

	// lock();
	sends(target, &func, sizeof(int));
	sends(target, &rpc_nonce, sizeof(int));
	if (func < RPC_variadic) {
		sends(target, input, inf->param_sz);
	} else {
		sends(target, input, *(int*)(input) + sizeof(int));
	}
	// unlock();
	
	
	struct rpc_wake rpc = { 0 };
	pthread_mutex_init(&rpc.mutex, NULL);
	pthread_mutex_lock(&rpc.mutex);
	pthread_cond_init(&rpc.cond, NULL);
	rpc.id = rpc_nonce;
	rpc.len = inf->response_sz;
	rpc.buffer = output;
	insert_rpc(&rpc);

	while (!rpc.responded) {
		pthread_cond_wait(&rpc.cond, &rpc.mutex);
	}
	printf("exited conditional wait\n");

	pthread_mutex_unlock(&rpc.mutex);
	pthread_mutex_destroy(&rpc.mutex);
	return 0;
}


struct remote_handler_args {
	int fd;
	int func;
	int nonce;
	void *args;
};

void *async_remote_handler(void *pargs)
{
	struct remote_handler_args *args = (struct remote_handler_args*)pargs;
	int fd = args->fd;
	int func = args->func;
	int nonce = args->nonce;
	void *in = args->args;
	request_socket = fd;
	free(pargs);
	remote_handler(fd, func, nonce, in);
}

void run(void* p_args, void* run_resp)
{
	pthread_t thread;
	struct thread_args *args = p_args;
	pthread_create(&thread, args->attr, args->start_routine, args->arg);

}

void remote_handler(int caller, int func, int nonce, void *args)
{
	printf("remote RPC no.%d\n", func);
	struct rpc_inf *inf = rpc_inf_table + func;

	void *resp = malloc(inf->response_sz);
	if (!resp) {
		perror("malloc");
		exit(EXIT_FAILURE);
		return;
	}
	rpc_inf_table[func].handler(args, resp);
	volatile int resp_code = RPC_resp;
	sends(caller, &resp_code, sizeof(resp_code));
	sends(caller, &nonce, sizeof(nonce));
	sends(caller, resp, inf->response_sz);

	free(args);
	free(resp);
}

void sched(void* p_args, void* sched_resp)
{
	struct thread_args *args = p_args;
	// finds a slave to run procedure
	// currently just a simple round robin approach
	static int i = 0;
	while (clients[i] != request_socket) {
		i = (i + 1) % NUM_CLIENTS;
	}
	remote(clients[i], RPC_run, &args, NULL);

}

int nop() 
{
	return 0;	
}

int (*real_main)(int, char**, char**);
int real_argc;
char** real_argv;
char** real_envp;


void exec_main(__attribute((unused)) void* vargs, void *vresp)
{
	if (!real_main) {
		real_main = nop;
	}
	int *pexitcode = vresp;
	*pexitcode = real_main(real_argc, real_argv, real_envp);
	printf("raking exit %d\n", *pexitcode);
}

void handle_s(int caller)
{
	request_socket = caller;
	int func = -1;

	recvs(caller, &func, sizeof(int));
	if (func < 0 || func >= RPC_MAX) {
		fprintf(stderr, "caller %d gave invalid func=%d\n", caller, func);
		close(caller);
		return;
	}
	if (func == RPC_resp) {
		printf("resp found\n");
		int id = -1;
		recvs(caller, &id, sizeof(int));
		if (id < 0) {
			fprintf(stderr, "caller %d gave invalid responseid=%d\n", caller, id);
			close(caller);
			return;
		}
		struct rpc_wake *rpc = pop_rpc(id);
		if (!rpc) {
			fprintf(stderr, "caller %d gave invalid func=%d\n", caller, func);
			close(caller);
			return;
		}
		recvs(caller, rpc->buffer, rpc->len);
		rpc->responded = true;
		pthread_cond_signal(&rpc->cond); //TODO handle read
		return;
	} 
	else if (func == RPC_notif) {
		// TODO handle async(?)
		return;
	}
	// otherwise its a request
	struct rpc_inf *inf = rpc_inf_table + func;
	printf("remote message was an RPC with no %d\n", func);
	int nonce = -1;
	recvs(caller, &nonce, sizeof(nonce));
	int len = inf->param_sz;
	void *args; 
	if (func < RPC_variadic) {
		args = malloc(inf->param_sz);
	} else {
		recvs(caller, &len, sizeof(int));
		args = malloc(len);
		if (args) *(int*)args = len;
	}

	if (!args) {
		perror("malloc");
		exit(EXIT_FAILURE);
		return;
	}
	recvs(caller, args, len);
	pthread_t worker;
	struct remote_handler_args *in = malloc(sizeof(struct remote_handler_args));
	if (!in) {
		perror("malloc");
		exit(EXIT_FAILURE);
		return;
	}
	in->fd = caller;
	in->func = func;
	in->nonce = nonce;
	in->args = args;

	pthread_create(&worker, NULL, async_remote_handler, in);
	pthread_detach(worker);
}

