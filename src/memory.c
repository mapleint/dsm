#include "memory.h"
#include "mesi.h"

struct page_entry pe_cache[NUM_ENTRIES];

struct page_entry *insert_page(void *addr, enum state st)
{
	for (int i = 0; i < NUM_ENTRIES; i++) {
		if (pe_cache[i].st == INVALID || pe_cache[i].addr == addr) {
			pe_cache[i].addr = addr;
			pe_cache[i].st = st;
			return &pe_cache[i];
		}
	}
	return NULL;
}

struct page_entry *find_page(void *addr)
{
	for (int i = 0; i < NUM_ENTRIES; i++) {
		if (pe_cache[i].addr == addr) {
			return &pe_cache[i];
		}
	}
	return NULL;

}

bool delete_page(void *addr)
{
	struct page_entry *entry = find_page(addr);
	if (!entry->addr) {
		return false;
	}
	entry->st = INVALID;
	entry->addr = NULL;
	return true;
}

