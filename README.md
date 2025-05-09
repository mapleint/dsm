# Signal-Based Distributed Shared Memory System 

Our signal-based distributed shared memory system is a segfault handling based implementation of shared memory for linux systems.

## System Design
dsm has two main components:
    n clients
    a centralized server
Each page is stored on a client, which contains a page cache that retains the state and addresses of the pages. 
Data is stored on clients locally, and they may "probe" the server for page locations (which may be stored on other clients); the server then asks every client whether they have the page, and then returns it.
We use signal handlers to check when pages are written/read to/from, and then modify the behavior to this distributed model.
Every program using our DSM system contains several clients, which are connected to the server. 

### Preload
We use LD_PRELOAD to function hook - in particular, intercept calls to mmap. Instead of allowing any protections to be set when we mmap a page, we instead always set the protection to PROT_NONE - this allows us to intercept any writes to the page, and handle the corresponding signal. 

### Illinois Protocol
In our initial proposal, we had intented to use the MOESI cache coherence protocol just with page granularity.
Unfortunately, the Owned state in MOESI adds complexity and more importantly only benefits non-writeback caches.
That is to say, since we send everything through 'main memory' (in this case, the server), we get no benefit from the added complexity

### cRPCs
We used unix sockets, explain
set up clients + server + remote calls
this subsection describes the rpc's in C

### Signal Based page-cache Coherence
We used sigaction to handle page faults. Whenever we allocated a relevant page, we always set the protections to none.
We then ask the server for the relevant page and get its location. Then, we will reprotect the page using mprotect to give it read/write permissions, 
and finally the corresponding read/write will be executed.

In hindsight, userfaultfd would have been a better idea here as it allows atomicity with respect to the state of a page.
We initially thought that userfaultd didn't give us access to read based faults, so we switched to signal handlers; however, we later found out that it was still possible.

#### Linux Lacks VirtualProtectEx()
Linux doesn't support virtualprotectEx (which allows one to change permissions of other address spaces).
Therefore, we can't have our clients be in different address sapces as we can directly change their protection levels from a client;
instead, we keep everything in one address space, which allows the server to map the protections.

## Known Issues

### Thread Safety
This program has a lot of pitfalls when it comes to thread safety, this is because we can not normally atomically write and write protect a page. As a result, if two threads are writing, it is possible that one thread will fault, the other will fail to fault because the page was unprotected in cstore and is immediately get clobbered by the memcpy in cstore. We do not know of a fix for this sans moving to userfaultfd, which explicitly support atomically copying a page and its data.

### Atomic Instructions
Atomic instructions on x86-64 are architecturally defined to still remain atomic on x86, even across cache or even page lines. Currently, our code doesn't explicitly handle instructions but they do fault as writes, and will only go through should we hold both pages.
In principle, an atomic instruction should work given the issue mentioned in the section above is resolved by using userfaultfd instead.

## Applications

### Matrix Multiplication

We can use our DSM server on matrix multiplication. Consider the following setup:
- $m$ by $n$ matrix $A$
- $n$ by $p$ matrix $B$
Then we can distribute the matrices $A[:m/2, :], A[m/2:, :], B[:, :p/2], B[:, :p/2]$ into $4$ memory locations,
and use $4$ clients to multiply these matrices ($A[:m/2, :] * B[:, :p/2]$ and so forth).
The locations of these matrices are sharded but we sill need to access them, hence the use of our DSM server.

## Future Work

