#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "rpc.h"
#include "client.c"

int main(int argc, char* argv[]) {
    shmem_init();

    struct sigaction sa = {
        .sa_sigaction = fault_handler,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGSEGV, &sa, NULL);

    //Getting shmem addresses of the arrays
    assert(argc == 7);
    int m1 = atoi(argv[0]);
    int n1 = atoi(argv[1]);
    int m2 = atoi(argv[2]);
    int n2 = atoi(argv[3]);
    assert(n1 == m2);
    char *addr1 = argv[0];
    char *addr2 = argv[1];
    char *dest = argv[2];
    
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
}
