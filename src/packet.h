#ifndef PACKET_H
#define PACKET_H

#include "moesi.h"

#define PAGE_SIZE 4096
/* constexpr int PAGE_SIZE = 4096; RIP C23 DREAM, 20(05|25) */

struct page {
	char data[PAGE_SIZE];
};

struct rw_req {
	void *begin;
};

enum packet_type {
	PROBE_READ_REQUEST,
	PROBE_WRITE_REQUEST,
	PAGE_FLUSH,
};

struct packet {
	enum packet_type type;
};

#endif /* PACKET_H */

