#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.h"

// Compilation:
// gcc "02 - Flock.c" -o flockc

// This program imitates work of 3 processes (parent and 2 childs) with some file with flock-blocking, which means
// that 2 processes cannot both write into file, but can both read file in the same time. However it cannot write and read in the same time.

int lock_write()
{
    int fd;
    if ((fd = open("/tmp/file.txt", O_RDWR | O_CREAT, 00700)) == -1)
    {
        printf("Access to READ&WRITE error.\n");
        return 0;
    }
    
    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
        printf("Access to LOCK error.\n");
        close(fd);
        return 0;
    }
        
    const char buf[] = "ABRACADABRA";
    write(fd, &buf, strlen(buf) + 1);
        
    printf("Write imitation 2 seconds...\n");
    sleep(2);
        
    close(fd);
    flock(fd, LOCK_UN);
    
    return 1;
}

int lock_read(pid_t pid)
{
    int fd;
    if ((fd = open("/tmp/file.txt", O_RDONLY, 0)) == -1)
    {
        return 0;
    }
    
    if (flock(fd, LOCK_SH | LOCK_NB) < 0)
    {
        printf("Access to LOCK error.\n");
        close(fd);
        return 0;
    }
    
    char buf[12];
    read(fd, &buf, 12);
    
    printf("Process %d read the file: %s\n", pid, buf);
    
    close(fd);
    flock(fd, LOCK_UN);
    
    return 1;
}

int main()
{    
    pid_t main_pid = getpid();
    
    printf("Start of process %d\n", main_pid);
    
    // Handle child process killing.
    handle_child_finishing();
    
    // Lock file for writing.
    if (lock_write())
    {
        printf("Write finished, file unblocked.\n");
    }   
    else
    {
        perror("Error of calling lock_write");
        exit(EXIT_FAILURE);
    }
    
    // Parent makes child 1.
    pid_t child_pid;
    if ((child_pid = fork()) > 0)
    {
        printf("PARENT: Start of child process %d.\n", child_pid);
        
        // Parent makes child 2.
        pid_t next_child_pid;
        if ((next_child_pid = fork()) > 0)
        {
            printf("PARENT: Start of child process %d.\n", next_child_pid);
            printf("PARENT: Parent process %d sleeps 5 seconds.\n", getpid());
            sleep(5);
        }
    }
    
    printf("Process %d tries to access the file...\n", getpid());
    
    int canread = 0;
    while(!canread)
    {
        canread = lock_read(getpid());
    }
    
    printf("End of process %d\n", getpid());
        
    exit(EXIT_SUCCESS);
}
