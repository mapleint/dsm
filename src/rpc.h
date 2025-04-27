#ifndef RPC_H
#define RPC_H
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>

struct socket {
	int fd;
	socklen_t len;
	union {
		struct sockaddr_un un;
		struct sockaddr_in in;
	};
};

struct socket create_in(const char *idk);
struct socket create_un(const char *path);

int binds(struct socket *s);
int listens(struct socket *s);
int accepts(struct socket *s);
int connects(struct socket *s);

int sends(int receiv, void *buf, unsigned len);
int recvs(int sender, void *buf, unsigned len);

int remote(int worker, int func_id, void *input, void *output);

struct ping_args {
	int kind;
	char str[4];
};

struct ping_resp {
	int kind;
	char str[4];
};

void ping(struct ping_args*, struct ping_resp*);

enum rpc {
	RPC_NA,
	RPC_ping,
};

struct rpc_inf {
	void (*handler)(void*,void*);
	int param_sz;
	int respoinse_sz;
};

struct rpc_inf rpc_inf_table[] = {
	{NULL, -1, -1},
	{ping, sizeof(struct ping_args), sizeof(struct ping_resp)},
};

#endif /* RPC_H */

