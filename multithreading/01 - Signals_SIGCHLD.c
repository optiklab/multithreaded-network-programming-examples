#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Common.h"

// To compile:
// gcc -std=gnu99 "01 - Signals_SIGCHLD.c" -o signals_sigchld

// Task:
// Create a program which creates a child process and handles SIGCHLD signal from its childs.

int main()
{    
    pid_t main_pid = getpid();
    
    printf("Start of process %d\n", main_pid);
    
    // Handle child process killing.
    handle_child_finishing();

    // Make child.
    pid_t child_pid;
    if((child_pid = fork()))
    {
        printf("PARENT: Started child process %d.\n", child_pid);
        
        printf("PARENT: Parent process works 1 second.\n");
        
        sleep(1);
    }
    else
    {
        // Do anything in child.
    }
    
    printf("End of process %d\n", getpid());
        
    exit(0);
}
