#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 

#include "Common.h"

// Compilation:
// gcc -std=gnu99 ErrorHandling.c LogF.c "04 - Process attributes.c" -o processAttrs

// Task:
// Work with process attributes.

static void printLimitValue(rlim_t limit)
{
    if (limit == RLIM_INFINITY)
        printf("RLIM_INFINITY");
    else
        printf("%llu", (unsigned long long)limit);
}

static bool readLimits(int resource, const char *name)
{
    struct rlimit r;
    ec_neg1( getrlimit(resource, &r) )
    printf("%s: ", name);
    printf(" current: ");
    printLimitValue(r.rlim_cur);
    printf(" max: ");
    printLimitValue(r.rlim_max);
    printf("\n");
    
    return true;
    
EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

static bool readAndChangeProcessLimits()
{
    printf("6 CHILD: Changing process limits...\n");
    struct rlimit r;
    int fd;
    char buf[500] = { 0 };
    if (sizeof(rlim_t) > sizeof(long long))
        printf("Warning: rlim_t > long long, results might be wrong!\n");
    
    ec_false( readLimits(RLIMIT_CORE, "6 CHILD: RLIMIT_CORE") )
    ec_false( readLimits(RLIMIT_CPU, "6 CHILD: RLIMIT_CPU") )
    ec_false( readLimits(RLIMIT_DATA, "6 CHILD: RLIMIT_DATA") )
    ec_false( readLimits(RLIMIT_FSIZE, "6 CHILD: RLIMIT_FSIZE") )
    ec_false( readLimits(RLIMIT_NOFILE, "6 CHILD: RLIMIT_NOFILE") )
    ec_false( readLimits(RLIMIT_STACK, "6 CHILD: RLIMIT_STACK") )
    
    ec_neg1( getrlimit(RLIMIT_FSIZE, &r) )
    
    r.rlim_cur = 500;
    
    ec_neg1( setrlimit(RLIMIT_FSIZE, &r) )
    ec_neg1( fd = open("tmp", O_WRONLY | O_CREAT | O_TRUNC, PERM_FILE) )
    ec_neg1( write(fd, buf, sizeof(buf)) )
    ec_neg1( write(fd, buf, sizeof(buf)) )
    printf("6 CHILD: 2 buffers has been written!\n");
    
    return true;
    
EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

static bool showProcessResourcesStat()
{
    struct rusage r;
    ec_neg1( getrusage(RUSAGE_SELF, &r) )
    printf("7 CHILD: user CPU time used %d \n", r.ru_utime);
    printf("7 CHILD: system CPU time used %d \n", r.ru_stime);
    printf("7 CHILD: max resident set size %d \n", r.ru_maxrss);
    printf("7 CHILD: integral shared size %d \n", r.ru_ixrss);
    printf("7 CHILD: integral unshared data (data segment) size %d \n", r.ru_idrss);
    printf("7 CHILD: integral unshared stack size %d \n", r.ru_isrss);
    printf("7 CHILD: page reclaims (soft page faults) %d \n", r.ru_minflt);
    printf("7 CHILD: hard page faults %d \n", r.ru_majflt);
    printf("7 CHILD: swaps / page file touches %d \n", r.ru_nswap);
    printf("7 CHILD: block input operations %d \n", r.ru_inblock);
    printf("7 CHILD: block output operations %d \n", r.ru_oublock);
    printf("7 CHILD: IPC messages sent %d \n", r.ru_msgsnd);
    printf("7 CHILD: IPC messages received %d \n", r.ru_msgrcv);
    printf("7 CHILD: signals received %d \n", r.ru_nsignals);
    printf("7 CHILD: voluntary context switches by process %d \n", r.ru_nvcsw);
    printf("7 CHILD: involuntary context switches by core %d \n", r.ru_nivcsw);
    
    return true;
    
EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

int main(int argc, char *argv[])
{
    printf("Enter to main!\n");
    printf("Create 1 child process...\n");
    pid_t pid;
    int status;
    switch(pid=fork())
    {
        case -1:
            perror("1 Error of calling fork");
            exit(EXIT_FAILURE);
        case 0:
            printf("1 CHILD: Child process %d!\n", getpid());
            printf("2 CHILD: Parent PID %d!\n", getppid());
            printf("3 CHILD: Who created me? User ID: %d\n", getuid());
            printf("3 CHILD: Who created me? Effective User ID: %d\n", geteuid());
            printf("3 CHILD: Who created me? Group ID: %d\n", getgid());
            printf("3 CHILD: Who created me? Effective Group ID: %d\n", getegid());
            
            // WARNING!! Works only under ROOT.
            // Changing root from / to some another directory (Example: restrict access to directories more than Web Server directories for Security).
            //printf("4 CHILD: Let's change Working Dir: %d\n", chdir(""));
            //printf("4 CHILD: Let's change Working Dir: %d\n", fchdir(""));
            //printf("4 CHILD: Let's change Root Dir: %d\n", chroot(""));
            // Process priority is in range N=[0, 39]. Nice returns N-20. In case of error Nice returns -1. If N == 19 ir returns -1 TOO :).
            printf("5 CHILD: Let's decrease Priority Level for N=5: %d (N-20)\n", nice(5));
            printf("5 CHILD: Let's increase Priority Level for N=-5: %d (N-20)\n", nice(-5));
            
            readAndChangeProcessLimits();
            
            showProcessResourcesStat();
            
            printf("8 CHILD: Wait 4 seconds...\n");
            sleep(4);
            printf("9 CHILD: Exit!\n");
            exit(EXIT_SUCCESS);
        default:
            printf("1 PARENT: Parent process %d!\n", getpid());
            printf("2 PARENT: Child PID %d\n", pid);
            printf("3 PARENT: Who created me? User ID: %d\n", getuid());
            printf("3 PARENT: Who created me? Group ID: %d\n", getgid());
            printf("3 PARENT: Who created me? Effective (under which permissions) User ID: %d\n", geteuid());
            printf("3 PARENT: Who created me? Effective (under which permissions) Group ID: %d\n", getegid());
            waitpid(pid, &status, 0);
            printf("4 PARENT: Child exit code: %d\n", WEXITSTATUS(status));
            printf("5 PARENT: Exit!\n");
    }

    printf("Exit process!\n");
    exit(EXIT_SUCCESS);
    
EC_CLEANUP_BGN
    exit(EXIT_FAILURE);
EC_CLEANUP_END
}