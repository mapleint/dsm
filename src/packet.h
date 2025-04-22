#ifndef PACKET_H
#define PACKET_H

#include "moesi.h"

#define PAGE_SIZE 4096
// constexpr int PAGE_SIZE = 4096; RIP C23 DREAM

struct page {
	char data[PAGE_SIZE];
};

enum packet_type {
	probe_read,
	probe_write,

	client_add,
};




#endif /* PACKET_H */
