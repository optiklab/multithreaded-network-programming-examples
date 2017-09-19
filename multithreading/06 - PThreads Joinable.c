#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "06 - PThreads Joinable BAD.c" -o joinable_threads

// Task:
// Simple example of an application working with 2 threads, where parent WAITS for a results from child.
// Thread 2 stops calculation if common value already more than limit.

static long x = 0;

static void* thread_func(void* arg)
{
    while (x < (long)arg)
    {
        printf("Thread 2, counter value %ld\n", ++x); // !!! WARNING!!! 2 Threads use the same variable. But increment operation is not atomic.
        sleep(1);
    }
    
    return (void *)x;
}

int main(void)
{
    pthread_t tid;
    void *status;
    
    assert(sizeof(long) <= sizeof(void *));
    ec_rv( pthread_create(&tid, NULL, thread_func, (void *)6) )

    while (x < 10)
    {
        printf("Thread 1, counter value %ld\n", ++x); // !!! WARNING!!! 2 Threads use the same variable. But increment operation is not atomic.
        sleep(2);
    }
    
    ec_rv( pthread_join(tid, &status) )
    printf("Return code of a Thread 2: %ld\n", (long)status);
    
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}