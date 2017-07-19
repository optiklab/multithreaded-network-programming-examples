#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "05 - PThreads.c" -o pthreads

// Task:
// Simple example of a counter working in 2 threads.

static long x = 0;

static void* thread_func(void* arg)
{
    while (true)
    {
        printf("Thread 2, counter value %ld\n", ++x);
        sleep(1);
    }
}

int main(void)
{
    pthread_t tid;

    ec_rv(pthread_create(&tid, NULL, thread_func, NULL))

    while (x < 10)
    {
        printf("Thread 1, counter value %ld\n", ++x);
        sleep(2);
    }
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}