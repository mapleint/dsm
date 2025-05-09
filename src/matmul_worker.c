#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "rpc.h"
#include "client.c"

struct spawn_args {
    int m1;
    int n1;
    int m2;
    int n2;
    char *addr1;
    char *addr2;
    char *dest;
};

void *spawn(void *func_args) {
    //Getting shmem addresses of the arrays
    //assert(argc == 7);
    struct spawn_args* args = (struct spawn_args *) func_args;
    int m1 = args->m1;
    int n1 = args->n1;
    int m2 = args->m2;
    int n2 = args->n2;
    assert(n1 == m2);
    char *addr1 = args->addr1;
    char *addr2 = args->addr2;
    char *dest = args->dest;
    
    
    //Getting arrays from addresses
    int **arr1 = (int**)addr1;
    int **arr2 = (int**)addr2;

    int** res_arr = malloc(m1 * sizeof(int*));
    for (int i = 0; i < m1; ++i) {
        res_arr[i] = malloc(n2*sizeof(int));
    }

    //Doing matmul

    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n2; ++j) {
            res_arr[i][j] = 0;
            for (int k = 0; k < n1; ++k) {
                res_arr[i][j] += arr1[i][k]*arr2[k][j];
            }
        }
    }

    //Copying to dest addr
    for (int i = 0; i < m1; ++i) {
        memcpy(dest + i*n2*sizeof(int), res_arr[i], n2*sizeof(int));
    }
    return NULL;
}

