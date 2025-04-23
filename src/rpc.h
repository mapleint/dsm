#ifndef RPC_H
#define RPC_H
#include <sys/socket.h>

struct socket {
	int type;
	int fd;
	union {
		struct sockaddr_un unix;
		struct sockaddr_in ip;
	};
};

socklen_t socklen(const struct socket* sock) {
	if (sock->type == AF_INET) {
		return sizeof(struct sockaddr_in);
	}
	return sizeof(struct sockaddr_un);
}

#endif /* RPC_H */

