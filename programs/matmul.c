#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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
                //printf("arr1[%d][%d] = %d\n", i, k, arr1[i*n1 + k]);
                //printf("arr2[%d][%d] = %d\n", k, j, arr2[k*n2 + j]);
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

int main(int argc, char* argv[]) {

    assert(argc = 4);
    int mat_n = atoi(argv[1]);
    int client_n = atoi(argv[2]);
    int max_num = atoi(argv[3]);
    srand(time(NULL));
    clock_t start, end;
    start = clock();

    int m1 = mat_n;
    int n1 = mat_n;
    int m2 = mat_n;
    int n2 = mat_n;

    //printf("Matrix 1: %d x %d\n", m1, n1);
    //printf("Matrix 2: %d x %d\n", m2, n2);

    int arr1_size = m1/client_n;
    int arr2_size = n2/client_n;

    // Getting random arrays
    int** arr1 = (int**)malloc(m1 * sizeof(int*));
    int** arr2 = (int**)malloc(m2 * sizeof(int*));
    for (int i = 0; i < m1; i++) {
        arr1[i] = (int*)malloc(n1 * sizeof(int));
    }
    for (int i = 0; i < m2; ++i) {
        arr2[i] = (int*)malloc(n2 * sizeof(int));
    }

    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n1; ++j) {
            arr1[i][j] = rand()%max_num;
        }
    }
    for (int i = 0; i < m2; ++i) {
        for (int j = 0; j < n2; ++j) {
            arr2[i][j] = rand()%max_num;
        }
    }


    void *p = mmap((void*)SHMEM_BASE, PAGE_SIZE*(client_n*client_n+2*client_n+1), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
   
    // Storing the shared memory
    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n1; ++j) {
            *(int*)(SHMEM_BASE + (i/arr1_size)*PAGE_SIZE + (i%arr1_size)*n1*sizeof(int) + j*sizeof(int)) = arr1[i][j];
        }
    }
    for (int i = 0; i < m2; ++i) {
        for (int j = 0; j < n2; ++j) {
            *(int*)(SHMEM_BASE + client_n*PAGE_SIZE + (j/arr2_size)*PAGE_SIZE + i*arr2_size*sizeof(int) + (j%arr2_size)*sizeof(int)) = arr2[i][j];
        }
    }
    //printf("Written to shared memory locations\n");

    for (int i = 0; i < client_n; ++i) {
        for (int j = 0; j < client_n; ++j) {
            pthread_t thread;
            struct spawn_args args = {arr1_size, n1, m2, arr2_size, 
                SHMEM_BASE + i*PAGE_SIZE, SHMEM_BASE + (client_n+j)*PAGE_SIZE,
                SHMEM_BASE + 2*client_n*PAGE_SIZE + i*client_n*PAGE_SIZE + j*PAGE_SIZE
            };
            pthread_create(&thread, NULL, spawn, &args);
            pthread_join(thread, NULL);
        }
    }

    int resarr[m1][n2];

    for (int i = 0; i < m1; ++i) {
        for (int j = 0; j < n2; ++j) {
            int x_shard = i/arr1_size;
            int y_shard = j/arr2_size;
            int x_pos = (i%arr1_size);
            int y_pos = (j%arr2_size);
            resarr[i][j] = *(int*)(SHMEM_BASE + 2*client_n*PAGE_SIZE + x_shard*client_n*PAGE_SIZE + y_shard*PAGE_SIZE + x_pos*arr2_size*sizeof(int) + y_pos*sizeof(int));
        }
    }

    /*
    printf("Arr 1:\n");
    for (int i = 0; i < m1; i++) {
        for (int j = 0; j < n1; j++) {
            printf("%d ", arr1[i][j]);
        }
        printf("\n");
    }
    printf("Arr 2:\n");
    for (int i = 0; i < m2; i++) {
        for (int j = 0; j < n2; j++) {
            printf("%d ", arr2[i][j]);
        }
        printf("\n");
    }

    printf("Result:\n");
    for (int i = 0; i < m1; i++) {
        for (int j = 0; j < n2; j++) {
            printf("%d ", resarr[i][j]);
        }
        printf("\n");
    }
    */
    
    end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    char filename[100];
    sprintf(filename,  "../experiments/matmul/%d_%d_%d.txt", mat_n, client_n, max_num);
    FILE * fptr = fopen(filename, "w");
    fprintf(fptr, "%f\n", time_spent);
    fclose(fptr);
}

