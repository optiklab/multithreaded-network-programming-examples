#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.h"

// Compilation:
// gcc "02 - Lock_via_file.c" -o lockviafile

// This program imitates work of 2 processes (parent and child) with some external resource.
// To access to resource both processes checks existance of a file, which means resource is still locked.
// If file is not exists anymore, then this means resource is free.

int unlock()
{
    unlink("/tmp/file.txt");
    return 1;
}

int lock()
{
    int fd;
    if ((fd = open("/tmp/file.txt", O_WRONLY | // Write to file
                                    O_CREAT |  // Create file if not exists or no block
                                    O_EXCL,    // Return error if file exists, but do not create one
                                    0)) == -1)
    {
        return 0;
    }
        
    close(fd);
    
    return 1;
}

int main()
{    
    pid_t main_pid = getpid();
    
    printf("Start of process %d\n", main_pid);
    
    // Handle killing child process.
    handle_child_finishing();
    
    // Make childs.
    pid_t child_pid;
    if((child_pid = fork()))
    {
        printf("PARENT: Started child process %d.\n", child_pid);
        
        sleep(1);
    }
    
    printf("Process %d tries to access the file...\n", getpid());
    
    int canread = 0;
    while(!canread)
    {
        canread = lock();
        
        if (canread)
        {
            printf("Process %d accessed the file and working with resource.\n", getpid());
            sleep(1);
            unlock();
            printf("File closed and unlocked by process %d.\n", getpid());
        }
        else
        {
            printf("Wait lock %d.\n", getpid());
            sleep(1);
        }
    }
    
    printf("End of process %d\n", getpid());
        
    exit(EXIT_SUCCESS);
}
