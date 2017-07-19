#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "08 - PThreads Joinable with Sync improved.c" -o joinable_threads_synced1

// Task:
// Simple example of an application working with 2 threads, where parent waits results from child.
// Thread 2 stops calculation if common value already more than limit.

// Access to variable is controlled with mutex, but located in the separate function.

static long get_and_incr_x(long incr)
{
    static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // Must be static, because initialization function maybe not thread-safe.
                                                            // And C++ may even use lazy initialization for mutex.
    static long x = 0;
    
    // rtn and incr variables are on stack - i.e. local
    long rtn;
    
    ec_rv( pthread_mutex_lock(&mtx) )
    rtn = x += incr;  // Static variable - i.e. global. Needs to be protected.
    ec_rv( pthread_mutex_unlock(&mtx) )
    
    return rtn;

EC_CLEANUP_BGN
    exit(EXIT_FAILURE);
EC_CLEANUP_END
}

static void* thread_func(void* arg)
{
    long val = 0;
    while (val < (long)arg)
    {
        val = get_and_incr_x(1);
        printf("Thread 2, counter value %ld\n", val);
        sleep(1);
    }
    
    return (void *)val;
}

int main(void)
{
    pthread_t tid;
    void *status;
    bool done;
    
    assert(sizeof(long) <= sizeof(void *));
    ec_rv( pthread_create(&tid, NULL, thread_func, (void *)6) )

    long val = 0;
    while (val < 10)
    {
        val = get_and_incr_x(1);
        printf("Thread 1, counter value %ld\n", val);
        sleep(2);
    }
    
    ec_rv( pthread_join(tid, &status) )
    printf("Return code of a Thread 2: %ld\n", (long)status);
    
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}