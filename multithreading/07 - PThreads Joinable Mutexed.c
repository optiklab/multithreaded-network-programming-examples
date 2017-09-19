#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "07 - PThreads Joinable Mutexed.c" -o joinable_threads_synced

// Task:
// Simple example of an application working with 2 threads, where parent WAITS for a results from child.
// Thread 2 stops calculation if common value already more than limit.

// Access to variable is controlled with mutex.

// Can be static, extern and even auto. In case of auto need to initialize with function pthread_mutex_init, not by a constant (for thread safety).
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static long x = 0;

static void* thread_func(void* arg)
{
    bool done;
    
    while (true)
    {
        ec_rv( pthread_mutex_lock(&mtx) )
        done = x <= (long)arg;
        ec_rv( pthread_mutex_unlock(&mtx) )
        if (done)
            break;
        ec_rv( pthread_mutex_lock(&mtx) )
        printf("Thread 2, counter value %ld\n", ++x);
        ec_rv( pthread_mutex_unlock(&mtx) )
        sleep(1);
    }
    
    return (void *)x;

EC_CLEANUP_BGN
    // To avoid dead lock: Thread 2 faulted on mutex_unlock, this Thread 2 is locked forever.
    (void)pthread_mutex_unlock(&mtx);
    EC_FLUSH("thread_func")
    return NULL;
EC_CLEANUP_END
}

int main(void)
{
    pthread_t tid;
    void *status;
    bool done;
    
    assert(sizeof(long) <= sizeof(void *));
    ec_rv( pthread_create(&tid, NULL, thread_func, (void *)6) )

    while (true)
    {
        ec_rv( pthread_mutex_lock(&mtx) )
        done = x >= 10;
        ec_rv( pthread_mutex_unlock(&mtx) )
        if (done)
            break;
        ec_rv( pthread_mutex_lock(&mtx) )
        printf("Thread 1, counter value %ld\n", ++x);
        ec_rv( pthread_mutex_unlock(&mtx) )
        sleep(2);
    }
    
    ec_rv( pthread_join(tid, &status) )
    printf("Return code of a Thread 2: %ld\n", (long)status);
    
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}