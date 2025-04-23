#ifndef RPC_H
#define RPC_H

enum connection {
	SOCKET,
	NETWORK,
};

struct local {
	char *path;

	int fd;


};

struct remote {
	int addr;
	int port;

	int fd;


};

struct remote_sys {
	enum connection type;
	union {
		struct local loc;
		struct remote rem;
	};
};

#endif /* RPC_H */

