#ifndef _ERROR_HANDLING_H_
#define _ERROR_HANDLING_H_

// System calls may:
// 1. Return -1 in case of error
// 2. Return NULL or something else, like SIG_ERR
// 3. Do not say about error at allocate

// errno - exact number of error happened (not always integer) from errno.h.
// stderror - text description of the error.
// perror() - prints text description of the error.

// After Error Handling it is necessary to clean up used resources.

// Both problems are solved here with macroses (all macroses are secure to use in multithreading):

extern const bool ec_in_cleanup;

typedef enum {EC_ERRNO = 0, EC_EAI = 1, EC_GETDATE = 2, EC_NONE = 3} EC_ERRTYPE;

// Starts Resources Cleanup if error actually happened.
#define EC_CLEANUP_BGN\
	ec_warn();\
	ec_cleanup_bgn:\
	{\
		bool ec_in_cleanup;\
		ec_in_cleanup = true;

// ec_in_cleanup variable allows to avoid possible INFINITE LOOP of error handling due to error appeared during Resources Cleanup.
// and it is always INTERNAL variable, because every Resources Cleanup should have its own state (not global).

// Close local content of Resources Cleanup after error.
#define EC_CLEANUP_END\
	}

// Main part of Resources Cleanup.
#define ec_cmp(var, errrtn)\
	{ /* Check that we are not in procedure right now */  \
		assert(!ec_in_cleanup);\
		if ((intptr_t)(var) == (intptr_t)(errrtn)) { /* Make sure error happened */ \
			ec_push(__func__, __FILE__, __LINE__, #var, errno, EC_ERRNO); /* Push location of error into stack */ \
			goto ec_cleanup_bgn;\
		}\
	}

// This is ec_cmp for fuctions which do not use errno and just return code of error (this code might be directly pushed into stack).
#define ec_rv(var)\
	{\
		int errrtn;\
		assert(!ec_in_cleanup);\
		if ((errrtn = (var)) != 0) {\
			ec_push(__func__, __FILE__, __LINE__, #var, errrtn, EC_ERRNO);\
			goto ec_cleanup_bgn;\
		}\
	}
    
// This is ec_rv for fuctions which do not use errno, but their return codes are not errno (that's why EC_EAI is used).
#define ec_ai(var)\
	{\
		int errrtn;\
		assert(!ec_in_cleanup);\
		if ((errrtn = (var)) != 0) {\
			ec_push(__func__, __FILE__, __LINE__, #var, errrtn, EC_EAI);\
			goto ec_cleanup_bgn;\
		}\
	}

#define ec_neg1(x) ec_cmp(x, -1)

// Not in book: 0 used instead of NULL to avoid warning from C++ compilers.
#define ec_null(x) ec_cmp(x, 0)
#define ec_zero(x) ec_null(x) /* not in book */
#define ec_false(x) ec_cmp(x, false)
#define ec_eof(x) ec_cmp(x, EOF)
#define ec_nzero(x)\
	{\
		if ((x) != 0)\
			EC_FAIL\
	}

// Is used for the errors happened w/o using of macroses above.
#define EC_FAIL ec_cmp(0, 0)

// Just go to cleanup.
#define EC_CLEANUP goto ec_cleanup_bgn;

// Prints error information while program not stopped yet (or not going to stop at all).
#define EC_FLUSH(str)\
	{\
		ec_print();\
		ec_reinit();\
	}
/*[]*/
#define EC_EINTERNAL INT_MAX

// Pushes error info and context into stack.
void ec_push(const char *fcn, const char *file, int line, const char *str, int errno_arg, EC_ERRTYPE type);

// Prints error info and context from stack.
void ec_print(void);

// Clears stack with errors info (forget all errors).
void ec_reinit(void);

// This function is a mark of error: so our Cleanup Procedure will be executed ONLY if error actually happened.
void ec_warn(void);

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>

// Decrypt errno number into a description of error.
char *errsymbol(const char *cat, int code, char **desc);

static struct {
    char *cat;
    intptr_t code;
    char *str;
    char *desc;
} errcodes[] =
{
    {"errno", (intptr_t)EPERM, "EPERM", ""},
    {"errno", (intptr_t)ENOENT, "ENOENT", ""},
    {"errno", (intptr_t)ESRCH, "ESRCH", ""},
    {"errno", (intptr_t)EINTR, "EINTR", ""},
    {"errno", (intptr_t)EIO, "EIO", ""},
    {"errno", (intptr_t)ENXIO, "ENXIO", ""},
    {"errno", (intptr_t)E2BIG, "E2BIG", ""},
    {"errno", (intptr_t)ENOEXEC, "ENOEXEC", ""},
    {"errno", (intptr_t)EBADF, "EBADF", ""},
    {"errno", (intptr_t)ECHILD, "ECHILD", ""},
    {"errno", (intptr_t)EAGAIN, "EAGAIN", ""},
    {"errno", (intptr_t)ENOMEM, "ENOMEM", ""},
    {"errno", (intptr_t)EACCES, "EACCES", ""},
    {"errno", (intptr_t)EFAULT, "EFAULT", ""},
    {"errno", (intptr_t)EBUSY, "EBUSY", ""},
    {"errno", (intptr_t)EEXIST, "EEXIST", ""},
    {"errno", (intptr_t)EXDEV, "EXDEV", ""},
    {"errno", (intptr_t)ENODEV, "ENODEV", ""},
    {"errno", (intptr_t)ENOTDIR, "ENOTDIR", ""},
    {"errno", (intptr_t)EISDIR, "EISDIR", ""},
    {"errno", (intptr_t)EINVAL, "EINVAL", ""},
    {"errno", (intptr_t)ENFILE, "ENFILE", ""},
    {"errno", (intptr_t)EMFILE, "EMFILE", ""},
    {"errno", (intptr_t)ENOTTY, "ENOTTY", ""},
    {"errno", (intptr_t)EFBIG, "EFBIG", ""},
    {"errno", (intptr_t)ENOSPC, "ENOSPC", ""},
    {"errno", (intptr_t)ESPIPE, "ESPIPE", ""},
    {"errno", (intptr_t)EROFS, "EROFS", ""},
    {"errno", (intptr_t)EMLINK, "EMLINK", ""},
    {"errno", (intptr_t)EPIPE, "EPIPE", ""},
    {"errno", (intptr_t)EDOM, "EDOM", ""},
    {"errno", (intptr_t)ERANGE, "ERANGE", ""},
    {"errno", (intptr_t)EDEADLK, "EDEADLK", ""},
    {"errno", (intptr_t)ENAMETOOLONG, "ENAMETOOLONG", ""},
    {"errno", (intptr_t)ENOLCK, "ENOLCK", ""},
    {"errno", (intptr_t)ENOSYS, "ENOSYS", ""},
    {"errno", (intptr_t)ENOTEMPTY, "ENOTEMPTY", ""},
    #ifndef _POSIX_SOURCE
    {"errno", (intptr_t)ENOTBLK, "ENOTBLK", ""},
    {"errno", (intptr_t)ETXTBSY, "ETXTBSY", ""},
    {"errno", (intptr_t)ENOTSOCK, "ENOTSOCK", ""},
    {"errno", (intptr_t)EDESTADDRREQ, "EDESTADDRREQ", ""},
    {"errno", (intptr_t)EMSGSIZE, "EMSGSIZE", ""},
    {"errno", (intptr_t)EPROTOTYPE, "EPROTOTYPE", ""},
    {"errno", (intptr_t)ENOPROTOOPT, "ENOPROTOOPT", ""},
    {"errno", (intptr_t)EPROTONOSUPPORT, "EPROTONOSUPPORT", ""},
    {"errno", (intptr_t)ESOCKTNOSUPPORT, "ESOCKTNOSUPPORT", ""},
    {"errno", (intptr_t)EOPNOTSUPP, "EOPNOTSUPP", ""},
    {"errno", (intptr_t)EPFNOSUPPORT, "EPFNOSUPPORT", ""},
    {"errno", (intptr_t)EAFNOSUPPORT, "EAFNOSUPPORT", ""},
    {"errno", (intptr_t)EADDRINUSE, "EADDRINUSE", ""},
    {"errno", (intptr_t)EADDRNOTAVAIL, "EADDRNOTAVAIL", ""},
    {"errno", (intptr_t)ENETDOWN, "ENETDOWN", ""},
    {"errno", (intptr_t)ENETUNREACH, "ENETUNREACH", ""},
    {"errno", (intptr_t)ENETRESET, "ENETRESET", ""},
    {"errno", (intptr_t)ECONNABORTED, "ECONNABORTED", ""},
    {"errno", (intptr_t)ECONNRESET, "ECONNRESET", ""},
    {"errno", (intptr_t)ENOBUFS, "ENOBUFS", ""},
    {"errno", (intptr_t)EISCONN, "EISCONN", ""},
    {"errno", (intptr_t)ENOTCONN, "ENOTCONN", ""},
    {"errno", (intptr_t)ESHUTDOWN, "ESHUTDOWN", ""},
    {"errno", (intptr_t)ETOOMANYREFS, "ETOOMANYREFS", ""},
    {"errno", (intptr_t)ETIMEDOUT, "ETIMEDOUT", ""},
    {"errno", (intptr_t)ECONNREFUSED, "ECONNREFUSED", ""},
    {"errno", (intptr_t)ELOOP, "ELOOP", ""},
    {"errno", (intptr_t)EWOULDBLOCK, "EWOULDBLOCK", ""},
    {"errno", (intptr_t)EALREADY, "EALREADY", ""},
    {"errno", (intptr_t)EINPROGRESS, "EINPROGRESS", ""},
    {"errno", (intptr_t)EHOSTDOWN, "EHOSTDOWN", ""},
    {"errno", (intptr_t)EHOSTUNREACH, "EHOSTUNREACH", ""},
    {"errno", (intptr_t)ESTALE, "ESTALE", ""},
    {"errno", (intptr_t)EUSERS, "EUSERS", ""},
    #endif /* _POSIX_SOURCE */
    {"errno", (intptr_t)EC_EINTERNAL, "EC_EINTERNAL", ""},
    {"sigaction", (intptr_t)SIG_DFL, "SIG_DFL", ""},
    {"sigaction", (intptr_t)SIG_IGN, "SIG_IGN", ""},
    {"sigaction", (intptr_t)SIG_ERR, "SIG_ERR", ""},
    {"sigaction_flags", (intptr_t)SA_NOCLDSTOP, "SA_NOCLDSTOP", ""},
    {"sigaction_flags", (intptr_t)SA_NOCLDWAIT, "SA_NOCLDWAIT", ""},
    {"sigaction_flags", (intptr_t)SA_NODEFER, "SA_NODEFER", ""},
    {"sigaction_flags", (intptr_t)SA_ONSTACK, "SA_ONSTACK", ""},
    {"sigaction_flags", (intptr_t)SA_RESETHAND, "SA_RESETHAND", ""},
    {"sigaction_flags", (intptr_t)SA_RESTART, "SA_RESTART", ""},
    {"sigaction_flags", (intptr_t)SA_SIGINFO, "SA_SIGINFO", ""},
    {"sigprocmask", (intptr_t)SIG_BLOCK, "SIG_BLOCK", ""},
    {"sigprocmask", (intptr_t)SIG_SETMASK, "SIG_SETMASK", ""},
    {"sigprocmask", (intptr_t)SIG_UNBLOCK, "SIG_UNBLOCK", ""},
    {"signal", (intptr_t)SIGABRT, "SIGABRT", "Process abort signal"},
    {"signal", (intptr_t)SIGALRM, "SIGALRM", "Alarm clock"},
    {"signal", (intptr_t)SIGBUS, "SIGBUS", "Access to undefined portion of memory object"},
    {"signal", (intptr_t)SIGCHLD, "SIGCHLD", "Child terminated, stopped, or continued."},
    {"signal", (intptr_t)SIGCONT, "SIGCONT", "Continue executing, if stopped"},
    {"signal", (intptr_t)SIGFPE, "SIGFPE", "Erroneous arithmetic operation"},
    {"signal", (intptr_t)SIGHUP, "SIGHUP", "Hangup"},
    {"signal", (intptr_t)SIGILL, "SIGILL", "Illegal instruction"},
    {"signal", (intptr_t)SIGINT, "SIGINT", "Terminal interrupt signal"},
    {"signal", (intptr_t)SIGKILL, "SIGKILL", "Kill (cannot be caught or ignored)"},
    {"signal", (intptr_t)SIGPIPE, "SIGPIPE", "Write on a pipe with no one to read it"},
    {"signal", (intptr_t)SIGQUIT, "SIGQUIT", "Terminal quit signal"},
    {"signal", (intptr_t)SIGSEGV, "SIGSEGV", "Invalid memory reference"},
    {"signal", (intptr_t)SIGSTOP, "SIGSTOP", "Stop executing (cannot be caught or ignored)"},
    {"signal", (intptr_t)SIGTERM, "SIGTERM", "Termination signal"},
    {"signal", (intptr_t)SIGTSTP, "SIGTSTP", "Terminal stop signal"},
    {"signal", (intptr_t)SIGTTIN, "SIGTTIN", "Background process attempting read"},
    {"signal", (intptr_t)SIGTTOU, "SIGTTOU", "Background process attempting write"},
    {"signal", (intptr_t)SIGUSR1, "SIGUSR1", "User-defined signal 1"},
    {"signal", (intptr_t)SIGUSR2, "SIGUSR2", "User-defined signal 2"},
    {"signal", (intptr_t)SIGPROF, "SIGPROF", "Profiling timer expired"},
    {"signal", (intptr_t)SIGSYS, "SIGSYS", "Bad system call"},
    {"signal", (intptr_t)SIGTRAP, "SIGTRAP", "Trace/breakpoint trap"},
    {"signal", (intptr_t)SIGURG, "SIGURG", "High bandwidth data is available at a socket"},
    {"signal", (intptr_t)SIGVTALRM, "SIGVTALRM", "Virtual timer expired"},
    {"signal", (intptr_t)SIGXCPU, "SIGXCPU", "CPU time limit exceeded"},
    {"signal", (intptr_t)SIGXFSZ, "SIGXFSZ", "File size limit exceeded"},
    {"fcntl", (intptr_t)O_APPEND, "O_APPEND", ""},
    {"fcntl", (intptr_t)O_CREAT, "O_CREAT", ""},
    {"fcntl", (intptr_t)O_EXCL, "O_EXCL", ""},
    {"fcntl", (intptr_t)O_NOCTTY, "O_NOCTTY", ""},
    {"fcntl", (intptr_t)O_NONBLOCK, "O_NONBLOCK", ""},
    {"fcntl", (intptr_t)O_TRUNC, "O_TRUNC", ""},
    {"fcntl_io", (intptr_t)O_RDONLY, "O_RDONLY", ""},
    {"fcntl_io", (intptr_t)O_RDWR, "O_RDWR", ""},
    {"fcntl_io", (intptr_t)O_WRONLY, "O_WRONLY", ""},
    {"clock", (intptr_t)CLOCKS_PER_SEC, "CLOCKS_PER_SEC", ""},
    {"stdfileno", (intptr_t)STDIN_FILENO, "STDIN_FILENO", ""},
    {"stdfileno", (intptr_t)STDOUT_FILENO, "STDOUT_FILENO", ""},
    {"stdfileno", (intptr_t)STDERR_FILENO, "STDERR_FILENO", ""},
    {"perm", (intptr_t)S_IRUSR, "S_IRUSR", ""},
    {"perm", (intptr_t)S_IWUSR, "S_IWUSR", ""},
    {"perm", (intptr_t)S_IXUSR, "S_IXUSR", ""},
    {"perm", (intptr_t)S_IRGRP, "S_IRGRP", ""},
    {"perm", (intptr_t)S_IWGRP, "S_IWGRP", ""},
    {"perm", (intptr_t)S_IXGRP, "S_IXGRP", ""},
    {"perm", (intptr_t)S_IROTH, "S_IROTH", ""},
    {"perm", (intptr_t)S_IWOTH, "S_IWOTH", ""},
    {"perm", (intptr_t)S_IXOTH, "S_IXOTH", ""},
    {"perm", (intptr_t)S_ISUID, "S_ISUID", ""},
    {"perm", (intptr_t)S_ISGID, "S_ISGID", ""},
    {"perm", (intptr_t)S_ISVTX, "S_ISVTX", ""},
    {"lseek", (intptr_t)SEEK_SET, "SEEK_SET", ""},
    {"lseek", (intptr_t)SEEK_CUR, "SEEK_CUR", ""},
    {"lseek", (intptr_t)SEEK_END, "SEEK_END", ""},
    {"st_mode", (intptr_t)S_IFMT, "S_IFMT", ""},
    {"st_mode", (intptr_t)S_IFBLK, "S_IFBLK", ""},
    {"st_mode", (intptr_t)S_IFCHR, "S_IFCHR", ""},
    {"st_mode", (intptr_t)S_IFDIR, "S_IFDIR", ""},
    {"st_mode", (intptr_t)S_IFIFO, "S_IFIFO", ""},
    {"st_mode", (intptr_t)S_IFLNK, "S_IFLNK", ""},
    {"st_mode", (intptr_t)S_IFREG, "S_IFREG", ""},
    {"st_mode", (intptr_t)S_IFSOCK, "S_IFSOCK", ""},
    {"access", (intptr_t)R_OK, "R_OK", ""},
    {"access", (intptr_t)W_OK, "W_OK", ""},
    {"access", (intptr_t)X_OK, "X_OK", ""},
    {"access", (intptr_t)F_OK, "F_OK", ""},
    {"fcntl_op", (intptr_t)F_DUPFD, "F_DUPFD", ""},
    {"fcntl_op", (intptr_t)F_GETFD, "F_GETFD", ""},
    {"fcntl_op", (intptr_t)F_SETFD, "F_SETFD", ""},
    {"fcntl_op", (intptr_t)F_GETFL, "F_GETFL", ""},
    {"fcntl_op", (intptr_t)F_SETFL, "F_SETFL", ""},
    {"fcntl_op", (intptr_t)F_GETOWN, "F_GETOWN", ""},
    {"fcntl_op", (intptr_t)F_SETOWN, "F_SETOWN", ""},
    {"fcntl_op", (intptr_t)F_GETLK, "F_GETLK", ""},
    {"fcntl_op", (intptr_t)F_SETLK, "F_SETLK", ""},
    {"fcntl_op", (intptr_t)F_SETLKW, "F_SETLKW", ""},
    {"fcntl_fd", (intptr_t)FD_CLOEXEC, "FD_CLOEXEC", ""},
    {"lockf", (intptr_t)F_LOCK, "F_LOCK", ""},
    {"lockf", (intptr_t)F_TLOCK, "F_TLOCK", ""},
    {"lockf", (intptr_t)F_TEST, "F_TEST", ""},
    {"lockf", (intptr_t)F_ULOCK, "F_ULOCK", ""},
    {"exit", (intptr_t)EXIT_SUCCESS, "EXIT_SUCCESS", ""},
    {"exit", (intptr_t)EXIT_FAILURE, "EXIT_FAILURE", ""},
    {"SysVIPC_mode", (intptr_t)IPC_CREAT, "IPC_CREAT", ""},
    {"SysVIPC_mode", (intptr_t)IPC_EXCL, "IPC_EXCL", ""},
    {"SysVIPC_mode", (intptr_t)IPC_NOWAIT, "IPC_NOWAIT", ""},
    {"SysVIPC_key", (intptr_t)IPC_PRIVATE, "IPC_PRIVATE", ""},
    {"SysVIPC_cmd", (intptr_t)IPC_RMID, "IPC_RMID", ""},
    {"SysVIPC_cmd", (intptr_t)IPC_STAT, "IPC_STAT", ""},
    {"SysVIPC_cmd", (intptr_t)IPC_SET, "IPC_SET", ""},
    {"SysVIPC_cmd", (intptr_t)GETNCNT, "GETNCNT", ""},
    {"SysVIPC_cmd", (intptr_t)GETZCNT, "GETZCNT", ""},
    {"SysVIPC_cmd", (intptr_t)GETPID, "GETPID", ""},
    {"SysVIPC_cmd", (intptr_t)GETVAL, "GETVAL", ""},
    {"SysVIPC_cmd", (intptr_t)SETVAL, "SETVAL", ""},
    {"SysVIPC_cmd", (intptr_t)GETALL, "GETALL", ""},
    {"SysVIPC_cmd", (intptr_t)SETALL, "SETALL", ""},
    {"SysVIPC_shm", (intptr_t)SHM_RND, "SHM_RND", ""},
    {"SysVIPC_shm", (intptr_t)SHM_RDONLY, "SHM_RDONLY", ""},
    {"mmap", (intptr_t)PROT_READ, "PROT_READ", ""},
    {"mmap", (intptr_t)PROT_WRITE, "PROT_WRITE", ""},
    {"mmap", (intptr_t)PROT_EXEC, "PROT_EXEC", ""},
    {"mmap_flags", (intptr_t)MAP_SHARED, "MAP_SHARED", ""},
    {"mmap_flags", (intptr_t)MAP_PRIVATE, "MAP_PRIVATE", ""},
    {"mmap_flags", (intptr_t)MAP_FIXED, "MAP_FIXED", ""},
    {"sigevent", (intptr_t)SIGEV_SIGNAL, "SIGEV_SIGNAL", ""},
    {"sigevent", (intptr_t)SIGEV_NONE, "SIGEV_NONE", ""},
    {"skt_domain", (intptr_t)AF_UNIX, "AF_UNIX", ""},
    {"skt_domain", (intptr_t)AF_INET, "AF_INET", ""},
    {"skt_domain", (intptr_t)AF_INET6, "AF_INET6", ""},
    {"skt_domain", (intptr_t)AF_UNSPEC, "AF_UNSPEC", ""},
    {"skt_type", (intptr_t)SOCK_DGRAM, "SOCK_DGRAM", ""},
    {"skt_type", (intptr_t)SOCK_RAW, "SOCK_RAW", ""},
    {"skt_type", (intptr_t)SOCK_SEQPACKET, "SOCK_SEQPACKET", ""},
    {"skt_type", (intptr_t)SOCK_STREAM, "SOCK_STREAM", ""},
    {"skt_level", (intptr_t)SOL_SOCKET, "SOL_SOCKET", ""},
    {"skt_option", (intptr_t)SO_ACCEPTCONN, "SO_ACCEPTCONN", ""},
    {"skt_option", (intptr_t)SO_BROADCAST, "SO_BROADCAST", ""},
    {"skt_option", (intptr_t)SO_DEBUG, "SO_DEBUG", ""},
    {"skt_option", (intptr_t)SO_DONTROUTE, "SO_DONTROUTE", ""},
    {"skt_option", (intptr_t)SO_ERROR, "SO_ERROR", ""},
    {"skt_option", (intptr_t)SO_KEEPALIVE, "SO_KEEPALIVE", ""},
    {"skt_option", (intptr_t)SO_LINGER, "SO_LINGER", ""},
    {"skt_option", (intptr_t)SO_OOBINLINE, "SO_OOBINLINE", ""},
    {"skt_option", (intptr_t)SO_RCVBUF, "SO_RCVBUF", ""},
    {"skt_option", (intptr_t)SO_RCVLOWAT, "SO_RCVLOWAT", ""},
    {"skt_option", (intptr_t)SO_RCVTIMEO, "SO_RCVTIMEO", ""},
    {"skt_option", (intptr_t)SO_REUSEADDR, "SO_REUSEADDR", ""},
    {"skt_option", (intptr_t)SO_SNDBUF, "SO_SNDBUF", ""},
    {"skt_option", (intptr_t)SO_SNDLOWAT, "SO_SNDLOWAT", ""},
    {"skt_option", (intptr_t)SO_SNDTIMEO, "SO_SNDTIMEO", ""},
    {"skt_option", (intptr_t)SO_TYPE, "SO_TYPE", ""},
    {"skt_backlog", (intptr_t)SOMAXCONN, "SOMAXCONN", ""},
    {"skt_msg_flag", (intptr_t)MSG_CTRUNC, "MSG_CTRUNC", ""},
    {"skt_msg_flag", (intptr_t)MSG_DONTROUTE, "MSG_DONTROUTE", ""},
    {"skt_msg_flag", (intptr_t)MSG_EOR, "MSG_EOR", ""},
    {"skt_msg_flag", (intptr_t)MSG_OOB, "MSG_OOB", ""},
    {"skt_msg_flag", (intptr_t)MSG_PEEK, "MSG_PEEK", ""},
    {"skt_msg_flag", (intptr_t)MSG_TRUNC, "MSG_TRUNC", ""},
    {"skt_msg_flag", (intptr_t)MSG_WAITALL, "MSG_WAITALL", ""},
    {"skt_shut", (intptr_t)SHUT_RD, "SHUT_RD", ""},
    {"skt_shut", (intptr_t)SHUT_RDWR, "SHUT_RDWR", ""},
    {"skt_shut", (intptr_t)SHUT_WR, "SHUT_WR", ""},
    {"skt_cmsg_type", (intptr_t)SCM_RIGHTS, "SCM_RIGHTS", ""},
    {"getaddrinfo", (intptr_t)AI_PASSIVE, "AI_PASSIVE", ""},
    {"getnameinfo", (intptr_t)NI_NOFQDN, "NI_NOFQDN", ""},
    {"getnameinfo", (intptr_t)NI_NUMERICHOST, "NI_NUMERICHOST", ""},
    {"getnameinfo", (intptr_t)NI_NAMEREQD, "NI_NAMEREQD", ""},
    {"getnameinfo", (intptr_t)NI_NUMERICSERV, "NI_NUMERICSERV", ""},
    {"getnameinfo", (intptr_t)NI_DGRAM, "NI_DGRAM", ""},
    {"sockopt_level", (intptr_t)IPPROTO_IP, "IPPROTO_IP", ""},
    {"sockopt_level", (intptr_t)IPPROTO_IPV6, "IPPROTO_IPV6", ""},
    {"sockopt_level", (intptr_t)IPPROTO_ICMP, "IPPROTO_ICMP", ""},
    {"sockopt_level", (intptr_t)IPPROTO_RAW, "IPPROTO_RAW", ""},
    {"sockopt_level", (intptr_t)IPPROTO_TCP, "IPPROTO_TCP", ""},
    {"sockopt_level", (intptr_t)IPPROTO_UDP, "IPPROTO_UDP", ""}
};
#endif /* _ERROR_HANDLING_H_ */