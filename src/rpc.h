#ifndef RPC_H
#define RPC_H
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>

#include "mesi.h"



typedef int (*pthread_create_t) (pthread_t *restrict thread,
	       	const pthread_attr_t *restrict attr,
		void *(*start_routine)(void*),
		void *restrict arg);

struct socket {
	int fd;
	socklen_t len;
	union {
		struct sockaddr_un un;
		struct sockaddr_in in;
	};
};

struct socket create_in(const char *ip, unsigned short port);
struct socket create_un(const char *path);

int binds(struct socket *s);
int listens(struct socket *s);
int accepts(struct socket *s);
int connects(struct socket *s);

int sends(int receiv, void *buf, unsigned len);
int recvs(int sender, void *buf, unsigned len);

int remote(int worker, int func_id, void *input, void *output);

struct ping_args {
	char str[5];
};

struct ping_resp {
	char str[5];
};

struct pr_args {
	char *addr;
};

struct pr_resp {
	enum state st;
	char page[4096];
};

struct pw_args {
	char *addr;
};

struct pw_resp {
	enum state st;
	char page[4096];
};

struct load_args {
	char *addr;
};

struct load_resp {
	enum state st;
	char page[4096];
};

struct store_args {
	char *addr;
};

struct store_resp {
	enum state st;
	char page[4096];
};

struct thread_args {
	const pthread_attr_t *restrict attr;
	void *(*start_routine)(void*);
	void *restrict arg;
};

void ping(void* /*struct ping_args*/, void* /*struct ping_resp*/);

void probe_read(void* /*struct pr_args*/, void* /*struct pr_resp*/);
void probe_write(void* /*struct pw_args*/, void* /*struct pw_resp*/);

void load(void* /*struct load_args*/, void* /*struct load_resp*/);
void store(void* /*struct store_args*/, void* /*struct store_resp*/);

void sched(void* /*struct sched_args*/, void* /*struct sched_resp*/);
void run(void* /*struct run_args*/, void* /*struct run_resp*/);
void exec_main(void*, void*); // you get the idea lmao

enum rpc {
	RPC_NA,
	RPC_resp,
	RPC_notif,

	RPC_ping,

	RPC_probe_read,
	RPC_probe_write,

	RPC_load,
	RPC_store,

	RPC_exec,

	RPC_run,
	RPC_sched,

	RPC_variadic,
	RPC_MAX,
};

struct rpc_inf {
	void (*handler)(void*,void*);
	int param_sz;
	int response_sz;
};

void handle_s(int caller);
void *async_remote_handler(void *caller_fd);

#endif /* RPC_H */

