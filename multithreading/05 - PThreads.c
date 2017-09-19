#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "05 - PThreads.c" -o pthreads

// Task:
// Simple example of a counter working in 2 threads. Example shows that sequence of ++ calls is not deterministic.

static long x = 0;

static void* thread_func(void* arg)
{
    while (true)
    {
        printf("Thread 2, counter value %ld\n", ++x); // !!! WARNING!!! 2 Threads use the same variable. But increment operation is not atomic.
        sleep(1);
    }
}

int main(void)
{
    pthread_t tid;

    // Use macros ec_rv to determine returned error (if happened), because pthread doesn't use errno variable to set error info.
    ec_rv(pthread_create(&tid,
        NULL, // Use attributes by default. To create attribute, call pthread_attr_setscope or pthread_attr_setstaoksize
        thread_func,
        NULL))

    while (x < 10)
    {
        printf("Thread 1, counter value %ld\n", ++x); // !!! WARNING!!! 2 Threads use the same variable. But increment operation is not atomic.
        sleep(2);
    }
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}