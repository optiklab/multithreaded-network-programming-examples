#include <pthread.h>

#include "Defs.h"

// Compilation:
// gcc -std=gnu99 -pthread ErrorHandling.c LogF.c "09 - PThreads Mutexed with Conditional Variables.c" -o threads_and_queue

// Task:
// Application working with 2 threads - one is putting data into queue, another one gets data from the queue.
// a. Threads to be stopped forcibly with pthread_cancel.
// b. Queue data do not contain mark of END of data flow for normal cases (we use (a) for both
// normal and abnormal termination). This is TO BE DONE.
// c. We may use Signals for normal and abnormal termination. But it has a problems:

// Access to queue is controlled with mutex and conditional variable.

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct node
{
    int n_number;
    struct node *n_next;
} *head = NULL;

static void cleanup_handler(void *arg)
{
    free(arg);
    (void)pthread_mutex_unlock(&mtx);
}

static void* thread_func(void* arg)
{
    struct node *p = NULL; // Should be initialized at least with NULL, since we use free.
    
    pthread_cleanup_push(cleanup_handler, p); // Set up cleanup handler for case of forced thread termination
                                              // to avoid dead lock on locked mutex.
    while (true)
    {
        ec_rv( pthread_mutex_lock(&mtx) )
        
        while (head == NULL) // Loop is used because pthread_cond_wait
                             // can be interrupted by signal (SIGINT, etc.)
                             // and we need to get back to conditional wait.        
            ec_rv( pthread_cond_wait(&cond, &mtx) ) // Conditional variable UNLOCKS mutex while waiting,
                                                    // but locks it back when condition happened.
        
        p = head;
        head = head->n_next;
        int number = p->n_number;
        free(p); // Free called here, because next point - printf(), might be a
                 // Point of Interruption (free() or malloc() cannot),
                 // and we need to make sure memory is freed.
        printf("Got number %d from the queue\n", number);
        ec_rv( pthread_mutex_unlock(&mtx) )
    }
    
    pthread_cleanup_pop(cleanup_handler, p); 
    return (void *)true;
    
EC_CLEANUP_BGN
    (void)pthread_mutex_unlock(&mtx);
    EC_FLUSH("thread_func")
    return (void *)false;
EC_CLEANUP_END
}

int main(void)
{
    pthread_t tid;
    int i;
    struct node *p;
    
    ec_rv( pthread_create(&tid, NULL, thread_func, NULL) )

    for (i = 0; i < 10; i++)
    {
        // Imitation of data receive here...
        ec_null( p = malloc(sizeof(struct node)) )
        p->n_number = i;
        
        // Put into queue.... Using mutex protection.
        ec_rv( pthread_mutex_lock(&mtx) )
        p->n_next = head;
        head = p;
        ec_rv( pthread_cond_signal(&cond) ) // Signal about data in the queue.
        ec_rv( pthread_mutex_unlock(&mtx) )
        sleep(1);
    }
    
    ec_rv( pthread_cancel(tid) )    
    ec_rv( pthread_join(tid, NULL) )
    printf("End of transmission\n");
    
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}