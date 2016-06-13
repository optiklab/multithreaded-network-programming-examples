#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Common.h"

// Compilation:
// gcc "03 - Nonamed_pipes_wc_logins.c" -o nonamedpipeslogins

// Task:
// Create a program which works just like the command
// $> who | wc -l

// Help:
// Create pipe: who -> stdout -> [pfd[1] -> pfd[0]] -> stdin -> wc -l
int main()
{
    printf("Result of operation is the same like if you type\n$>who | wc -l\n");
    
    int pfd[2];
    
    // Handle child process killing.
    handle_child_finishing();
    
    pipe(pfd);

    if (!fork())
    {
        // Since descriptors are shared between the parent and child,
        // we should always be sure to close the end of pipe we aren't concerned with.
        // On a technical note, the EOF will never be returned
        // if the unnecessary ends of the pipe are not explicitly closed.
        close(STDOUT_FILENO);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        close(pfd[0]);
        execlp("who", "who", NULL);
    }
    else
    {
        close(STDIN_FILENO);
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[1]);
        close(pfd[0]);
        execlp("wc", "wc", "-l", NULL);
    }
    exit(0);
}