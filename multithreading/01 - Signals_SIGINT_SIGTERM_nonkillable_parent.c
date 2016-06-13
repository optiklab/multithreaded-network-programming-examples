#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Common.h"

pid_t child_pid;

// Compilation:
// gcc "01 - Signals_SIGINT_SIGTERM_nonkillable_parent.c" -o nonkillable_parent

// Task:
// Create a program which creates a child process and this child process can be killed by SIGTERM or SIGINT.
// However, parent should not be killed by first SIGTERM/SIGINT. It may be killed in further signals.

void cant_stop_me_handler(int sig)
{
    if (child_pid)
    {
        raise(-9); // or kill(-9, child_pid);
        printf("SIGCHLD Child: killed %d.\n", child_pid);
    }
    else
    {
        printf("SIGCHLD Parent: Ha-ha, can't kill me this time :)! (try again)\n");
    }
}

void cant_stop_me_handler_int(int sig)
{
    if (child_pid)
    {
        raise(-9); // or kill(-9, child_pid);
        printf("SIGINT Child: killed %d.\n", child_pid);
    }
    else
    {
        printf("SIGINT Parent: Ha-ha, can't kill me this time :)! (try again)\n");
    }
}

int main()
{
    pid_t main_pid = getpid();
    
    printf("Start of process %d forever.\n", main_pid);
    
    // Do no stop by SIGTERM.
    struct sigaction cant_stop_me_signal;
    cant_stop_me_signal.sa_handler = cant_stop_me_handler;
    sigemptyset(&cant_stop_me_signal.sa_mask);
    cant_stop_me_signal.sa_flags = SA_RESETHAND; // Non-permanent handler. Handle only first time...
    
    if (sigaction(SIGTERM, &cant_stop_me_signal, 0) == -1)
    {
        perror("Error of calling sigaction in parent.");
        exit(EXIT_FAILURE);
    }

    // Do no stop by SIGINT.
    struct sigaction cant_stop_me_signal1;
    cant_stop_me_signal1.sa_handler = cant_stop_me_handler_int;
    sigemptyset(&cant_stop_me_signal1.sa_mask);
    cant_stop_me_signal1.sa_flags = SA_RESETHAND; // Non-permanent handler. Handle only first time...
    
    if (sigaction(SIGINT, &cant_stop_me_signal1, 0) == -1)
    {
        perror("Error of calling sigaction in parent.");
        exit(EXIT_FAILURE);
    }

    // Make child.
    if((child_pid = fork()))
    {
        printf("PARENT: Start of child process %d forever. Press Ctrl+C OR write 'kill -9 %d' to terminate child.\n", child_pid, child_pid);
    }
    else
    {
        // Do anything in child.
    }

    while(1)
    {
        // Forever... until someone press Ctrl+C or write 'kill -9 xxxx'
    }
    
    printf("End of process %d\n", getpid());
        
    exit(0);
}
