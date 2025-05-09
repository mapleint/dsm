#ifndef MEMORY_H
#define MEMORY_H

#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mesi.h"

#define NUM_ENTRIES 256

struct page_entry {
	void *addr;
	enum state st;
};

#define PAGE_SIZE 4096

#define SHMEM_BASE ((void*)0x69696969000LL)
#define SHMEM_LEN  PAGE_SIZE * 256

static inline void r_prot(void *page)
{
	mprotect(page, PAGE_SIZE, PROT_READ);
}

static inline void rw_prot(void *page)
{
	if (mprotect(page, PAGE_SIZE, PROT_READ | PROT_WRITE)) {
		printf("mprotect %p\n", page);
		perror("mprotect");
	}
}

static inline void no_prot(void *page)
{
	mprotect(page, PAGE_SIZE, PROT_NONE);
}


static void *shmem;

static inline void *page_align(void *addr)
{
	uintptr_t page = (uintptr_t)addr;
	page &= ~0xFFFLL;
	return (void*)page;
}

static void shmem_init()
{
	shmem = mmap(SHMEM_BASE, SHMEM_LEN, PROT_NONE, 
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

	if (shmem == MAP_FAILED) {
		perror("mmap failed");
		exit(EXIT_FAILURE);
	}
}


struct page_entry *insert_page(void *addr, enum state st);

struct page_entry *find_page(void *addr);

bool delete_page(void *addr);

#endif /* MEMORY_H */

