#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 

#include "Common.h"

// Compilation:
// gcc -std=gnu99 "03 - Parent Process Die.c" -o parentDie

// Task:
// Use fork to create a child process. Emulate parent process died and Parent PID should become Init (1) or some other?

const char* get_process_name_by_pid(const int pid)
{
    char* name = (char*)calloc(1024,sizeof(char));
    if(name)
    {
        sprintf(name, "/proc/%d/cmdline",pid);
        FILE* f = fopen(name,"r");
        if(f)
        {
            size_t size;
            size = fread(name, sizeof(char), 1024, f);
            if(size>0)
            {
                if('\n'==name[size-1])
                {
                    name[size-1]='\0';
                }
            }
            fclose(f);
        }
    }
    return name;
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
            printf("3 CHILD: Wait 3 seconds...\n");
            sleep(3);
            pid_t ppid = getppid();
            printf("4 CHILD: Looks like PARENT died! What is my parent pid? %d\n", ppid); // TODO Find Parent Process Name.
            printf("5 CHILD: My parent now is: %s\n", get_process_name_by_pid(ppid)); // TODO Find Parent Process Name.
            printf("6 CHILD: Wow! Cool! Exit!\n");
            exit(EXIT_SUCCESS);
        default:
            printf("1 PARENT: Parent process %d!\n", getpid());
            printf("2 PARENT: Child PID %d\n", pid);
            printf("3 PARENT: Wait 2 seconds...\n");
            sleep(2);
            printf("4 PARENT: Dying... bye-bye child process!\n");
    }
}