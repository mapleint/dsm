#include <sys/socket.h>
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

