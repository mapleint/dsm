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

// int sends(struct socket *s, void *buffer, size_t len);
int binds(struct socket *s);
int listens(struct socket *s);
int accepts(struct socket *s);
int connects(struct socket *s);

#endif /* RPC_H */

