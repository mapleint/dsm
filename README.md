# dsm 

dsm is a segfault handling based implementation of shared memory for linux systems

## System Design
dsm has two main components:
    clients
    a centralized server
### Preload


### Illinois Protocol
In our initial proposal, we had intented to use the MOESI cache coherence protocol just with page granularity.
Unfortunately, the Owned state in MOESI adds complexity and more importantly only benefits non-writeback caches.
That is to say, since we send everything through 'main memory' (in this case, the server), we get no benefit from the added complexity

### Function Hooking
this subsection describes function hooking in C

### cRPCs
this subsection describes the rpc's in C

### Signal Based page-cache Coherence
In hindsight, userfaultfd would have been a better idea here as it allows atomicity with respect to the state of a page.

#### Linux Lacks VirtualProtectEx()
this subsection describes the lack of virtualprotectEx and why that necessitates a LD PRELOAD

## Known Issues

### Thread Safety
This program has a lot of pitfalls when it comes to thread safety, this is because we can not normally atomically write and write protect a page. As a result, if two threads are writing, it is possible that one thread will fault, the other will fail to fault because the page was unprotected in cstore and is immediately get clobbered by the memcpy in cstore. We do not know of a fix for this sans moving to userfaultfd, which explicitly support atomically copying a page and its data.

### Atomic Instructions
Atomic instructions on x86-64 are architecturally defined to still remain atomic on x86, even across cache or even page lines. Currently, our code doesn't explicitly handle instructions but they do fault as writes, and will only go through should we hold both pages.
In principle, an atomic instruction should work given the issue mentioned in the section above is resolved by using userfaultfd instead.

## Future Work

