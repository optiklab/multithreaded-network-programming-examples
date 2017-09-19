#ifndef _UNIFIED_EVENT_MANAGER_H_
#define _UNIFIED_EVENT_MANAGER_H_

#include <pthread.h>
#include <sys/select.h>
#include <mqueue.h>
#include <semaphore.h>

/*[UEM_TYPE]*/
enum UEM_TYPE {
	UEM_SVMSG,      /* System V message */
	UEM_PXMSG,      /* POSIX message */
	UEM_SVSEM,      /* System V semaphore */
	UEM_PXSEM,      /* POSIX semaphore */
	UEM_FD_READ,    /* file-descriptor set - read */
	UEM_FD_WRITE,   /* file-descriptor set - write */
	UEM_FD_ERROR,   /* file-descriptor set  - error */
	UEM_SIG,        /* signal */
	UEM_PROCESS,    /* process */
	UEM_HEARTBEAT,  /* heartbeat */
	UEM_NONE        /* none */
};
/*[uem_reg]*/
struct uem_reg {
	enum UEM_TYPE ur_type;          /* type of registration */
	pthread_t ur_tid;               /* thread ID */
	union {
		int ur_mqid;				/* System V message-queue ID */
		struct {
			int s_semid;			/* System V semaphore-set ID */
			struct sembuf *s_sops;	/* semaphore operations */
		} ur_svsem;
		mqd_t ur_mqd;				/* POSIX message-queue descriptor */
		sem_t *ur_sem;				/* POSIX semaphore */
		int ur_signum;				/* signal number */
		pid_t ur_pid;				/* process ID */
		long ur_usecs;				/* microseconds (for heartbeat) */
		fd_set ur_fdset;			/* file-descriptor set */
	} ur_resource;
	void *ur_data;					/* data to be queued with event */
	size_t ur_size;					/* size (used for various purposes) */
};
/*[uem_event]*/
struct uem_event {
	struct uem_reg *ue_reg;   /* information filled during event registering */
	void *ue_buf;             /* information returned */
	ssize_t ue_result;        /* process end code */
	int ue_errno;             /* error, if happened */
	struct uem_event *ue_next;/* next event in the queue */
};
/*[]*/

bool uem_bgn(void);
bool uem_end(void);
bool uem_register_svmsg(int mqid, size_t msgsize, void *data);
bool uem_register_svsem(int semid, struct sembuf *sops, size_t nsops, void *data);

bool uem_register_pxmsg(mqd_t mqd, void *data);
bool uem_register_pxsem(sem_t *sem, void *data);

bool uem_register_signal(int signum, void *data);
bool uem_register_process(pid_t pid, void *data); /* also array of pids? */
bool uem_register_fdset(int nfds, fd_set *fdset, enum UEM_TYPE type, void *data);
bool uem_register_heartbeat(long usecs, void *data);

struct uem_event *uem_wait(void);
void uem_free(struct uem_event *e);
bool uem_unregister(struct uem_event *e);

#endif