#include "Defs.h"
#include "UnifiedEventManager.h"

#include <sys/msg.h>
#include <sys/sem.h>

/*[top]*/
static pthread_mutex_t uem_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t uem_cond_event = PTHREAD_COND_INITIALIZER;
static struct uem_event *event_head;
/*[new_reg]*/ 

static struct uem_reg *new_reg(void)
{
    struct uem_reg *p;

    ec_null( p = calloc(1, sizeof(struct uem_reg)) )
    return p;

EC_CLEANUP_BGN
    return NULL;
EC_CLEANUP_END
}
/*[]*/
/* Must contain no cancellation points. */
/*[queue_event]*/
static bool queue_event(struct uem_event *e)
{
    struct uem_event *cur;

    ec_rv( pthread_mutex_lock(&uem_mtx) )
    if (event_head == NULL)
    {
        event_head = e;
    }
    else
    {
        for (cur = event_head; cur->ue_next != NULL; cur = cur->ue_next)
        {
            /* queue same error only once */
            if (e->ue_errno != 0 && cur->ue_reg->ur_type == e->ue_reg->ur_type && cur->ue_errno == e->ue_errno)
            {
                ec_rv( pthread_mutex_unlock(&uem_mtx) )
                uem_free(e);
                return true;
            }
        }
        cur->ue_next = e;
    }
    ec_rv( pthread_cond_signal(&uem_cond_event) )
    ec_rv( pthread_mutex_unlock(&uem_mtx) )
    return true;

EC_CLEANUP_BGN
    (void)pthread_mutex_unlock(&uem_mtx);
    return false;
EC_CLEANUP_END
}

/*[dequeue_event]*/
static bool dequeue_event(struct uem_reg *p)
{
    struct uem_event *cur, *prev, *next;

    ec_rv( pthread_mutex_lock(&uem_mtx) )
    for (cur = event_head; cur != NULL; cur = next)
    {
        next = cur->ue_next;
        if (cur->ue_reg == p)
        {
            if (prev == NULL)
                event_head = next;
            else
                prev->ue_next = next;
            uem_free(cur);
        }
        else
            prev = cur;
    }
    ec_rv( pthread_mutex_unlock(&uem_mtx) )
    return true;

EC_CLEANUP_BGN
    (void)pthread_mutex_unlock(&uem_mtx);
    return false;
EC_CLEANUP_END
}
/*[cleanup_handler]*/
static void cleanup_handler(void *arg)
{
    (void)uem_free((struct uem_event *)arg);
}
/*[]*/

static void free_svmsg(struct uem_event *e)
{
    free(e->ue_buf);
}
/*[]*/

static void *thread_svmsg(void *arg)
{
    struct uem_event *e = NULL;

    pthread_cleanup_push(cleanup_handler, e);
    while (true)
    {
        ec_null( e = calloc(1, sizeof(struct uem_event)) )
        e->ue_reg = (struct uem_reg *)arg;
        ec_null( e->ue_buf = malloc(e->ue_reg->ur_size) )
        
        if ((e->ue_result = msgrcv(e->ue_reg->ur_resource.ur_mqid, e->ue_buf, e->ue_reg->ur_size - sizeof(long), 0, 0)) == -1)
        {
            e->ue_errno = errno;
        }
            
        ec_false( queue_event(e) )
    }
    pthread_cleanup_pop(false);
    return NULL;

EC_CLEANUP_BGN
    uem_free(e);
    EC_FLUSH("thread_svmsg")
    return NULL;
EC_CLEANUP_END
}

bool uem_register_svmsg(int mqid, size_t msgsize, void *data)
{
    struct uem_reg *p;

    ec_null( p = new_reg() )
    
    p->ur_type = UEM_SVMSG;
    p->ur_resource.ur_mqid = mqid;
    p->ur_size = msgsize;
    p->ur_data = data;
    
    ec_rv( pthread_create(&p->ur_tid, NULL, thread_svmsg, p) )
    
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

static void free_svsem_reg(struct uem_reg *p)
{
}

static void *thread_svsem(void *arg)
{
    struct uem_event *e = NULL;

    pthread_cleanup_push(cleanup_handler, e);
    while (true)
    {
        ec_null( e = calloc(1, sizeof(struct uem_event)) )
        
        e->ue_reg = (struct uem_reg *)arg;
        if ((e->ue_result = semop(e->ue_reg->ur_resource.ur_svsem.s_semid, e->ue_reg->ur_resource.ur_svsem.s_sops, e->ue_reg->ur_size)) == -1)
        {
            e->ue_errno = errno;
        }
        
        ec_false( queue_event(e) )
    }
    pthread_cleanup_pop(false);
    
    return NULL;

EC_CLEANUP_BGN
    uem_free(e);
    EC_FLUSH("thread_svsem")
    return NULL;
EC_CLEANUP_END
}

bool uem_register_svsem(int semid, struct sembuf *sops, size_t nsops,
  void *data)
{
    struct uem_reg *p;

    ec_null( p = new_reg() )
    p->ur_type = UEM_SVSEM;
    p->ur_resource.ur_svsem.s_semid = semid;
    ec_null( p->ur_resource.ur_svsem.s_sops = calloc(1, sizeof(struct sembuf) * nsops) )
    memcpy(p->ur_resource.ur_svsem.s_sops, sops, sizeof(struct sembuf) * nsops);
    p->ur_size = nsops;
    p->ur_data = data;
    
    ec_rv( pthread_create(&p->ur_tid, NULL, thread_svsem, p) )
    
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

static void *thread_pxmsg(void *arg)
{
	struct uem_event *e = NULL;
	struct mq_attr attr;

	pthread_cleanup_push(cleanup_handler, e);
	while (true)
    {
		ec_null( e = calloc(1, sizeof(struct uem_event)) )
		e->ue_reg = (struct uem_reg *)arg;
		ec_neg1( mq_getattr(e->ue_reg->ur_resource.ur_mqd, &attr) )
		ec_null( e->ue_buf = malloc(attr.mq_msgsize) )
		if ((e->ue_result = mq_receive(e->ue_reg->ur_resource.ur_mqd, e->ue_buf, attr.mq_msgsize, NULL)) == -1)
        {
			e->ue_errno = errno;
        }
		ec_false( queue_event(e) )
	}
    
	pthread_cleanup_pop(false);
	return NULL;

EC_CLEANUP_BGN
	uem_free(e);
	EC_FLUSH("thread_pxmsg")
	return NULL;
EC_CLEANUP_END
}

bool uem_register_pxmsg(mqd_t mqd, void *data)
{
	struct uem_reg *p;

	ec_null( p = new_reg() )
	p->ur_type = UEM_PXMSG;
	p->ur_resource.ur_mqd = mqd;
	p->ur_data = data;
	ec_rv( pthread_create(&p->ur_tid, NULL, thread_pxmsg, p) )
	return true;

EC_CLEANUP_BGN
	return false;
EC_CLEANUP_END
}

static void *thread_pxsem(void *arg)
{
	struct uem_event *e = NULL;

	pthread_cleanup_push(cleanup_handler, e);
    
	while (true)
    {
		ec_null( e = calloc(1, sizeof(struct uem_event)) )
		e->ue_reg = (struct uem_reg *)arg;
		if ((e->ue_result = sem_wait(e->ue_reg->ur_resource.ur_sem)) == -1)
        {
			e->ue_errno = errno;
        }
		ec_false( queue_event(e) )
	}
    
	pthread_cleanup_pop(false);
	return NULL;

EC_CLEANUP_BGN
	uem_free(e);
	EC_FLUSH("thread_pxsem")
	return NULL;
EC_CLEANUP_END
}

bool uem_register_pxsem(sem_t *sem, void *data)
{
	struct uem_reg *p;

	ec_null( p = new_reg() )
	p->ur_type = UEM_PXSEM;
	p->ur_resource.ur_sem = sem;
	p->ur_data = data;
	ec_rv( pthread_create(&p->ur_tid, NULL, thread_pxsem, p) )
	return true;

EC_CLEANUP_BGN
	return false;
EC_CLEANUP_END
}

/*
    Problem is that this thread calls select in a loop, and select returns if the condition is met. It does not block waiting for something new to happen. Solutions:

    1. Queue same event only once. Still causes a lot of wasted CPU.
    2. Sleep for a while. Causes poor responsiveness.
    3. Change the API to temporarily remove a ready file descriptor from the set.
*/
static void *thread_fdset(void *arg)
{
    struct uem_event *e = NULL;
    struct uem_reg *p = (struct uem_reg *)arg;
    fd_set fdset, *fdset_read = NULL, *fdset_write = NULL, *fdset_error = NULL;
    int i;

    pthread_cleanup_push(cleanup_handler, e);
    while (true)
    {
        /*
            Pre-allocate first event so that errno from select can be returned without an allocation.
        */
        ec_null( e = calloc(1, sizeof(struct uem_event)) )
        e->ue_reg = p;
        fdset = p->ur_resource.ur_fdset;
        
        switch(p->ur_type)
        {
        case UEM_FD_READ:
            fdset_read = &fdset;
            break;
        case UEM_FD_WRITE:
            fdset_write = &fdset;
            break;
        case UEM_FD_ERROR:
            fdset_error = &fdset;
            break;
        default:
            errno = EINVAL;
            EC_FAIL
        }
        
        if (select(p->ur_size, fdset_read, fdset_write, fdset_error, NULL) == -1)
        {
            e->ue_errno = errno;
            ec_false( queue_event(e) )
        }
        else
        {
            for (i = 0; i < p->ur_size; i++)
            {
                if (FD_ISSET(i, &fdset))
                {
                    struct uem_event *cur;

                    ec_rv( pthread_mutex_lock(&uem_mtx) )
                    for (cur = event_head; cur != NULL; cur = cur->ue_next)
                    {
                        if (cur->ue_reg->ur_type == p->ur_type && cur->ue_result == i)
                        {
                            break;
                        }
                    }
                    ec_rv( pthread_mutex_unlock(&uem_mtx) )
                    if (cur != NULL)
                    {
                        continue;
                    }
                    
                    if (e == NULL)
                    {
                        ec_null( e = calloc(1, sizeof(struct uem_event)) )
                        e->ue_reg = p;
                    }
                    e->ue_result = i;
                    ec_false( queue_event(e) )
                    e = NULL;
                }
            }
        }
    }
    pthread_cleanup_pop(false);
    return NULL;

EC_CLEANUP_BGN
    (void)pthread_mutex_unlock(&uem_mtx);
    uem_free(e);
    EC_FLUSH("thread_fdset")
    return NULL;
EC_CLEANUP_END
}

bool uem_register_fdset(int nfds, fd_set *fdset, enum UEM_TYPE type,
  void *data)
{
    struct uem_reg *p;

    switch (type)
    {
    case UEM_FD_READ:
    case UEM_FD_WRITE:
    case UEM_FD_ERROR:
        ec_null( p = new_reg() )
        p->ur_type = type;
        p->ur_resource.ur_fdset = *fdset;
        p->ur_size = nfds;
        p->ur_data = data;
        ec_rv( pthread_create(&p->ur_tid, NULL, thread_fdset, p) )
        return true;
    default:
        errno = EINVAL;
        EC_FAIL
    }

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

static void *thread_signal(void *arg)
{
    struct uem_event *e = NULL;
    sigset_t set;
    int signum;

    pthread_cleanup_push(cleanup_handler, e);
    ec_neg1( sigemptyset(&set) )
    ec_neg1( sigaddset(&set,
      ((struct uem_reg *)arg)->ur_resource.ur_signum) )
    while (true)
    {
        ec_null( e = calloc(1, sizeof(struct uem_event)) )
        e->ue_reg = (struct uem_reg *)arg;
        e->ue_errno = sigwait(&set, &signum);
        if (e->ue_errno == 0)
            e->ue_result = signum;
        ec_false( queue_event(e) )
    }
    pthread_cleanup_pop(false);
    return NULL;

EC_CLEANUP_BGN
    uem_free(e);
    EC_FLUSH("thread_signal")
    return NULL;
EC_CLEANUP_END
}

bool uem_register_signal(int signum, void *data)
{
    struct uem_reg *p;
    sigset_t set;

    ec_neg1( sigemptyset(&set) )
    ec_neg1( sigaddset(&set, signum) )
    ec_rv( pthread_sigmask(SIG_BLOCK, &set, NULL) )
    ec_null( p = new_reg() )
    p->ur_type = UEM_SIG;
    p->ur_resource.ur_signum = signum;
    p->ur_data = data;
    ec_rv( pthread_create(&p->ur_tid, NULL, thread_signal, p) )
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

/*
    Problem on systems that implemented threads as processes is that the pid that we want to wait on is not our child.

    A way to implement this is to keep track of all the pids to be waited for and use the heartbeat event to wake up every so often to do waitpids for them without creating a thread. Left as an exercise.

    What is here works fine on Solaris.
*/

/*[thread_process]*/
static void *thread_process(void *arg)
{
    struct uem_event *e = NULL;

    pthread_cleanup_push(cleanup_handler, e);
    ec_null( e = calloc(1, sizeof(struct uem_event)) )
    e->ue_reg = (struct uem_reg *)arg;
    int result = 0;
    if (waitpid(e->ue_reg->ur_resource.ur_pid, &result, 0) == -1)
    {        
        e->ue_errno = errno;
    }
    
    e->ue_result = result;
    
    ec_false( queue_event(e) )
    pthread_cleanup_pop(false);
    return NULL;

EC_CLEANUP_BGN
    uem_free(e);
    EC_FLUSH("thread_process")
    return NULL;
EC_CLEANUP_END
}

/*[uem_register_process]*/
bool uem_register_process(pid_t pid, void *data)
{
    struct uem_reg *p;

    ec_null( p = new_reg() )
    p->ur_type = UEM_PROCESS;
    p->ur_resource.ur_pid = pid;
    p->ur_size = 0;
    p->ur_data = data;
    ec_rv( pthread_create(&p->ur_tid, NULL, thread_process, p) )
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}
/*[]*/

static void *thread_heartbeat(void *arg)
{
    struct uem_event *e = NULL;
    struct timespec tspec;

    pthread_cleanup_push(cleanup_handler, e);
    tspec.tv_sec = ((struct uem_reg *)arg)->ur_resource.ur_usecs / 1000000;
    tspec.tv_nsec = (((struct uem_reg *)arg)->ur_resource.ur_usecs %
      1000000) * 1000;
    while (true)
    {
        ec_null( e = calloc(1, sizeof(struct uem_event)) )
        e->ue_reg = (struct uem_reg *)arg;

        ec_neg1( nanosleep(&tspec, NULL) )
        ec_false( queue_event(e) )
    }
    pthread_cleanup_pop(false);
    return NULL;

EC_CLEANUP_BGN
    uem_free(e);
    EC_FLUSH("thread_heartbeat")
    return NULL;
EC_CLEANUP_END
}

bool uem_register_heartbeat(long usecs, void *data)
{
    struct uem_reg *p;

    ec_null( p = new_reg() )
    p->ur_type = UEM_HEARTBEAT;
    p->ur_resource.ur_usecs = usecs;
    p->ur_size = 0;
    p->ur_data = data;
    ec_rv( pthread_create(&p->ur_tid, NULL, thread_heartbeat, p) )
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}
/*[uem_wait]*/
struct uem_event *uem_wait(void)
{
    struct uem_event *e = NULL;

    ec_rv( pthread_mutex_lock(&uem_mtx) )
    
    while (event_head == NULL)
    {
        ec_rv( pthread_cond_wait(&uem_cond_event, &uem_mtx) )
    }
    
    e = event_head;
    event_head = event_head->ue_next;
    ec_rv( pthread_mutex_unlock(&uem_mtx) )
    return e;

EC_CLEANUP_BGN
    (void)pthread_mutex_unlock(&uem_mtx);
    return NULL;
EC_CLEANUP_END
}
/*[]*/
void uem_free(struct uem_event *e)
{
    if (e != NULL)
    {
        switch (e->ue_reg->ur_type)
        {
        case UEM_SVMSG:
            free_svmsg(e);
            break;
        case UEM_PXMSG:
            break;
        case UEM_SVSEM:
            break;
        case UEM_PXSEM:
            break;
        case UEM_FD_READ:
        case UEM_FD_WRITE:
        case UEM_FD_ERROR:
            break;
        case UEM_SIG:
            break;
        case UEM_PROCESS:
            break;
        case UEM_HEARTBEAT:
            break;
        case UEM_NONE:
            break;
        }
        free(e);
    }
}

bool uem_unregister(struct uem_event *e)
{
    ec_rv( pthread_cancel(e->ue_reg->ur_tid) )
    ec_false( dequeue_event(e->ue_reg) )
    
    switch (e->ue_reg->ur_type)
    {
    case UEM_SVMSG:
        break;
    case UEM_PXMSG:
        break;
    case UEM_SVSEM:
        free_svsem_reg(e->ue_reg);
        break;
    case UEM_PXSEM:
        break;
    case UEM_FD_READ:
    case UEM_FD_WRITE:
    case UEM_FD_ERROR:
        break;
    case UEM_SIG:
        break;
    case UEM_PROCESS:
        break;
    case UEM_HEARTBEAT:
        break;
    case UEM_NONE:
        break;
    }
    
    free(e->ue_reg);
    printf("thread cancelled\n");
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

bool uem_bgn(void)
{
    sigset_t set;

    ec_neg1( sigemptyset(&set) )
    ec_neg1( sigfillset(&set) )
    ec_neg1( sigdelset(&set, SIGINT) ) /* convenient for debugging */
    ec_rv( pthread_sigmask(SIG_SETMASK, &set, NULL) )
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

bool uem_end(void)
{
    return true;
} 
