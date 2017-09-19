#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "Common.h"

// Compilation:
// gcc -std=gnu99 ErrorHandling.c LogF.c "01 - Fork Processes And End Differently.c" -o forkEndProcesses

// Task:
// Use fork to create a child processes, which stop working with different reasons.

void display_status(pid_t pid, int status)
{
    if (pid != 0)
        printf("Process %d: ", (long)pid);
    if (WIFEXITED(status))
    {
        printf("Exit code: %d\n", WEXITSTATUS(status));
    }
    else
    {
        char *desc;
        char *signame = errsymbol("signal", WTERMSIG(status), &desc); // 301

        if (desc != NULL && desc[0] == '?')
            desc = signame;

        if (signame != NULL && signame[0] == '?')
            printf("Signal #%d", WTERMSIG(status));
        else
            printf("%s", desc);

        if (WCOREDUMP(status))
            printf(" - memory dump reset");

        if (WIFSTOPPED(status))
            printf(" (paused)");
        
        printf("\n");
    }
}

static bool wait_and_display(void)
{
    pid_t wpid;
    int status;
    ec_neg1( wpid = waitpid(-1, &status, 0));
    display_status(wpid, status);
    return true;

EC_CLEANUP_BGN
    return false;
EC_CLEANUP_END
}

int main(int argc, char *argv[])
{
    printf("Enter to main!\n");
    pid_t pid;

    // Case 1: explicit call _exit.
    if (fork() == 0)
    {
        _exit(123); // Child.
    }
    ec_false(wait_and_display()); // Parent.

    // Case 2: fail stop.
    if (fork() == 0)
    {
        int a, b = 0; // Child.
        a = 1 / b; // Divide by zero
        _exit(EXIT_SUCCESS);
    }
    ec_false(wait_and_display()); // Parent.

    // Case 3: signal.
    if ((pid=fork()) == 0)
    {
        sleep(100);
        _exit(EXIT_SUCCESS);
    }
    ec_neg1(kill(pid, SIGHUP));
    ec_false(wait_and_display()); // Parent

    printf("Exit main!\n");
    exit(EXIT_SUCCESS);

EC_CLEANUP_BGN
    exit(EXIT_FAILURE);
EC_CLEANUP_END
}