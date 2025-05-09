#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "memory.h"

// Define a function pointer type for printf
typedef int (*printf_t)(const char *format, ...);

static printf_t original_printf = NULL;

// Custom printf implementation
int printf(const char *format, ...) {
    // Initialize original_printf if it's NULL
    if (!original_printf) {
        original_printf = (printf_t)dlsym(RTLD_NEXT, "vprintf");
        if (!original_printf) {
            fprintf(stderr, "Error: Could not find original printf function.\n");
            return -1;
        }
    }
    
    // fprintf(stderr, "[Hooked printf] "); // Print a message indicating the hook is active

    va_list args;
    char *str = strdup(format);
    str[0]='k';
    va_start(args, format);
    int result = original_printf(str, args); // Call the original printf
    va_end(args);

    free (str);
    return result;
}

typedef void* (*mmap_t) (void *addr, size_t len, int prot, int flags, int fd, off_t offset);

static mmap_t original_mmap = NULL;

void* mmap( void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    if (!original_mmap) {
        original_mmap = (mmap_t)dlsym(RTLD_NEXT, "mmap");
        if (!original_mmap) {
            fprintf(stderr, "Error, could not find the original mmap function");
            return (void*)(-1);
        }
    }

    return original_mmap(addr, len, prot, PROT_NONE, fd, offset);
}



/*

#include <stdlib.h>


void __attribute__((constructor)) init(void)
{
	printf("preload running\n");
	exit(0);
}
*/
