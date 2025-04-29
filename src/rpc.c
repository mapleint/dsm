#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "rpc.h"

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


struct rpc_inf rpc_inf_table[] = {
	{NULL, -1, -1},
	{ping, sizeof(struct ping_args), sizeof(struct ping_resp)},
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
	sends(caller, resp, inf->param_sz);
	free(resp);
}

