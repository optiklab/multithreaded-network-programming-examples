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
    struct sigaction kill_child_signal;
    kill_child_signal.sa_handler = kill_child_handler;
    sigemptyset(&kill_child_signal.sa_mask);
    kill_child_signal.sa_flags = SA_RESTART; // Permanent handler.
    
    if (sigaction(SIGCHLD, &kill_child_signal, 0) == -1)
    {
        perror("Error of calling sigaction");
        exit(EXIT_FAILURE);
    }

    // Make child.
    pid_t child_pid;
    if((child_pid = fork()))
    {
        printf("Started child process %d.\n", child_pid);
        
        printf("Parent process works 1 second.\n");
        
        sleep(1);
    }
    else
    {
        // Do anything in child.
    }
    
    printf("End of process %d\n", getpid());
        
    exit(0);
}
