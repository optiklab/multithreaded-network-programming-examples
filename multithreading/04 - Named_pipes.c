#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Common.h"

// Compilation:
// gcc "04 - Named_pipes.c" -o namedpipes

// Task:
// Create Named Pipe aka FIFO to do write on one end and read on another end.

int main()
{
    char * myfifo = "/tmp/myfifo";
    mkfifo(myfifo, 0666);
    
    // Handle child process killing.
    handle_child_finishing();
    
    pid_t childpid;
    if((childpid = fork()) == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(childpid == 0)
    {
        char string[] = "Hello, world!";
        int fd;
        if ((fd = open(myfifo, O_WRONLY)))
        {
            write(fd, string, (strlen(string)+1));
            close(fd);
        }

        printf("Child process %d done write and unlink fifo.\n", getpid());
        unlink(myfifo);
    }
    else
    {
        printf("Parent process %d creates child %d\n", getpid(), childpid);
        
        char readbuffer[80];
        int fd;
        if ((fd = open(myfifo, O_RDONLY)))
        {
            read(fd, readbuffer, 80);            
            printf("Parent process %d received string: %s\n", getpid(), readbuffer);
            close(fd);
        }
        unlink(myfifo);
    }
    
    printf("Exit process %d\n", getpid());
    exit(0);
}