#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "../src/memory.h"

struct spawn_args {
    int m1;
    int n1;
    int m2;
    int n2;
    char *addr1;
    char *addr2;
    char *dest;
};

void *spawn(void* func_args) {
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
    int *arr1 = (int*)addr1;
    int *arr2 = (int*)addr2;

    int** res_arr = malloc(m1 * sizeof(int*));
    for (int i = 0; i < m1; ++i) {
        res_arr[i] = malloc(n2*sizeof(int));
    }

    //Doing matmul
    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n2; ++j) {
            res_arr[i][j] = 0;
            for (int k = 0; k < n1; ++k) {
                res_arr[i][j] += arr1[i*n1 + k]*arr2[k*n2 + j];
            }
        }
    }

    //Copying to dest addr
    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n2; ++j) {
            *(int*)(dest + i*n2*sizeof(int) + j*sizeof(int)) = res_arr[i][j];
        }
    }

    return NULL;
}

#define SHMEM_00 SHMEM_BASE + PAGE_SIZE
#define SHMEM_01 SHMEM_BASE + 2*PAGE_SIZE
#define SHMEM_10 SHMEM_BASE + 3*PAGE_SIZE
#define SHMEM_11 SHMEM_BASE + 4*PAGE_SIZE

#define dest_00 SHMEM_BASE + 5*PAGE_SIZE
#define dest_01 SHMEM_BASE + 6*PAGE_SIZE
#define dest_10 SHMEM_BASE + 7*PAGE_SIZE
#define dest_11 SHMEM_BASE + 8*PAGE_SIZE

int m1 = 4;
int n1 = 4;
int m2 = 4;
int n2 = 4;

int arr1[4][4] = {
    {4, 3, 2, 1},
    {1, 2, 6, 4},
    {5, 0, 1, 9},
    {3, 8, 20, 4},
};

int arr2[4][4] = {
    {8, 1, 3, 9},
    {5, 4, 5, 6}, 
    {1, 1, 9, 3},
    {4, 4, 7, 2},
};

int main() {
    void *p = mmap((void*)SHMEM_BASE, PAGE_SIZE*32, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
    //printf("%p\n", p);
    //printf("SHMEM_00: %p\n", SHMEM_00);
    //printf("SHMEM_01: %p\n", SHMEM_01);
    //printf("SHMEM_10: %p\n", SHMEM_10);
    //printf("SHMEM_11: %p\n", SHMEM_11);
    // Storing the shared memory
    for (int i = 0; i < m1/2; ++i) {
        for (int j = 0; j < n1; ++j) {
            *(int*)(SHMEM_00 + i*n1*sizeof(int) + j*sizeof(int)) = arr1[i][j];
        }
    }
    for (int i = m1/2; i < m1; ++i) {
        for (int j = 0; j < n1; ++j) {
            *(int*)(SHMEM_01 + (i-m1/2)*n1*sizeof(int) + j*sizeof(int)) = arr1[i][j];
        }
    }
    for (int i = 0; i < m2; ++i) {
        for (int j = 0; j < n2/2; ++j) {
            *(int*)(SHMEM_10 + i*n2/2*sizeof(int) + j*sizeof(int)) = arr2[i][j];
        }
        for (int j = n2/2; j < n2; ++j) {
            *(int*)(SHMEM_11 + i*n2/2*sizeof(int) + (j-n2/2)*sizeof(int)) = arr2[i][j];
        }
    }

    //printf("Written to shared memory locations\n");


    pthread_t thread_00; 
    pthread_t thread_01; 
    pthread_t thread_10; 
    pthread_t thread_11; 
    
    struct spawn_args args_00 = {m1/2, n1, m2, n2/2, SHMEM_00, SHMEM_10, dest_00};
    pthread_create(&thread_00, NULL, spawn, &args_00);
    struct spawn_args args_01 = {m1/2, n1, m2, n2/2, SHMEM_00, SHMEM_11, dest_01};
    pthread_create(&thread_01, NULL, spawn, &args_01);
    struct spawn_args args_10 = {m1/2, n1, m2, n2/2, SHMEM_01, SHMEM_10, dest_10};
    pthread_create(&thread_10, NULL, spawn, &args_10);
    struct spawn_args args_11 = {m1/2, n1, m2, n2/2, SHMEM_01, SHMEM_11, dest_11};
    pthread_create(&thread_11, NULL, spawn, &args_11);

    pthread_join(thread_00, NULL);
    pthread_join(thread_01, NULL);
    pthread_join(thread_10, NULL);
    pthread_join(thread_11, NULL);

    int resarr[m1][n2];
    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n2; ++j) {
            if (i < m1/2 && j < n2/2) 
                resarr[i][j] = *(int*)(dest_00 + i*n2/2 * sizeof(int) + j*sizeof(int));
            if (i < m1/2 && j >= n2/2)
                resarr[i][j] = *(int*)(dest_01 + (i)*sizeof(int)*n2/2 + (j - n2/2)*sizeof(int));
            if (i >= m1/2 && j < n2/2)
                resarr[i][j] = *(int*)(dest_10 + (i - m1/2)*sizeof(int)*n2/2 + j*sizeof(int));
            if (i >= m1/2 && j >= n2/2)
                resarr[i][j] = *(int*)(dest_11 + (i-m1/2)*sizeof(int)*n2/2 + (j-n2/2)*sizeof(int));
        }
    }

    for (int i = 0; i < m1; i++) {
        for (int j = 0; j < n2; j++) {
            printf("%d ", resarr[i][j]);
        }
        printf("\n");
    }

}

