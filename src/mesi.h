#ifndef MOESI_H
#define MOESI_H

#include "packet.h"

enum state {
    INVALID,
    MODIFIED,
    SHARED,
    EXCLUSIVE,
/*
OWNED, Owned state not useful with central server
*/
};

/* {probe read = 0, write = 1} */


#endif /* MOESI_H */
