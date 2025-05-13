#include "client.h"

#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>

// Define a function pointer type for printf
typedef int (*printf_t)(const char *format, ...);

static printf_t original_printf = NULL;

extern int (*real_main)(int, char**, char**);
extern int real_argc;
extern char **real_argv;
extern char **real_envp;

int listen_loop(int argc, char **argv, char **envp)
{
	real_argv = argv;
	real_envp = envp;
	printf("listen loop entered\n");
	struct pollfd fds[1] = { 0 };
	fds[0].fd = s.fd, fds[0].events = POLLIN;
	while (true) {
		poll(fds, 1, -1);
		short socket_revents = fds[0].revents;
		if (socket_revents & POLLIN) {
			/* server packet */
			printf("server rpc detected\n");
			handle_s(s.fd);
		}
	}
	return 0;

}

int remote_pthread_create(const pthread_attr_t *restrict attr,
		void *(*start_routine)(void*),
		void *restrict arg)
{
	struct thread_args args = { 
		.attr = attr, 
		.start_routine = start_routine,
	       	.arg = arg };
	remote(s.fd, RPC_sched, &args, NULL);

}

extern pthread_create_t original_pthread_create;
int pthread_create(pthread_t *restrict thread,
		const pthread_attr_t *restrict attr,
		void *(*start_routine)(void*),
		void *restrict arg)
{
	printf("bro really tried threading in the big 2025\n");
	if (!original_pthread_create) {
		original_pthread_create = (pthread_create_t)dlsym(RTLD_NEXT, "pthread_create");
		if (!original_pthread_create) {
			fprintf(stderr, "Error: Could not find original pthread_create function.\n");
			return -1;
		}
	}
	return original_pthread_create(thread, attr, start_routine, arg);
}

// hook __libc_start_main
int __libc_start_main(
	int (*main)(int, char **, char **),
	int argc,
	char **ubp_av,
	void (*init)(void),
	void (*fini)(void),
	void (*rtld_fini)(void),
	void *stack_end
) {
	real_main = main;

	// Resolve real __libc_start_main
	typeof(&__libc_start_main) orig_libc_start_main;
	orig_libc_start_main = dlsym(RTLD_NEXT, "__libc_start_main");

	// Call it with our fake main
	return orig_libc_start_main(listen_loop, argc, ubp_av, init, fini, rtld_fini, stack_end);
}

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

    return original_mmap(addr, len, PROT_NONE, flags, fd, offset);
}

void __attribute__((constructor)) init(void)
{
	printf("initializer running\n");
	shmem_init();
	// Initializing the signal handler
	struct sigaction sa = {
		.sa_sigaction = fault_handler,
		.sa_flags = SA_SIGINFO,
	};
	sigaction(SIGSEGV, &sa, NULL);

	s = create_un(DEFAULT_SERVER_FILE);
	if (connects(&s)) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
	printf("connection successful: entering libc\n");
}

