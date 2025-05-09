#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <matmul_worker.c>

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

int main(int argc, char* argv[]) {
    shmem_init();

    struct sigaction sa = {
        .sa_sigaction = fault_handler,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGSEGV, &sa, NULL);

    // Storing the shared memory
    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n1/2; ++j) {
            *(int*)(SHMEM_00 + i*m1*sizeof(int) + j) = arr1[i][j];
        }
        for (int j = n1/2; j < n1; ++j) {
            *(int*)(SHMEM_01 + i*m1*sizeof(int) + j-n1/2) = arr1[i][j];
        }
    }
    for (int i = 0; i < m2; ++i) {
        for (int j = 0; j < n2/2; ++j) {
            *(int*)(SHMEM_10 + i*m2*sizeof(int) + j) = arr2[i][j];
        }
        for (int j = n2/2; j < n2; ++j) {
            *(int*)(SHMEM_11 + i*m2*sizeof(int) + j-n2/2) = arr2[i][j];
        }
    }

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
                resarr[i][j] = *(int*)(dest_00 + i*m1/2 * sizeof(int) + j);
            if (i < m1/2 && j >= n2/2)
                resarr[i][j] = *(int*)(dest_01 + (i)*sizeof(int) + (j - n2/2));
            if (i >= m1/2 && j < n2/2)
                resarr[i][j] = *(int*)(dest_10 + (i - m1/2)*sizeof(int) + j);
            if (i >= m1/2 && j >= n2/2)
                resarr[i][j] = *(int*)(dest_11 + (i-m1/2)*sizeof(int) + (j-n2/2));
        }
    }

    for (int i = 0; i < m1; i++) {
        for (int j = 0; j < n2; j++) {
            printf("%d ", resarr[i][j]);
        }
        printf("\n");
    }

}
