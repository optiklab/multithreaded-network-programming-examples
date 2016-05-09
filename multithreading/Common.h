#include <sys/wait.h>

#define EXIT_FAILURE 1

void kill_child_handler(int sig)
{
    int status;
    pid_t done = waitpid(-1, // Any child
                         &status,
                         0); // Blocked mode.
    if (done == -1)
    {
        printf("No more child processes.\n");
    }
    else
    {
        short isNormalTermination = WIFEXITED(status);
        if (!isNormalTermination ||
            // WEXITSTATUS should be used only if normal termination = true.
            (isNormalTermination && WEXITSTATUS(status) != 0))
        {
            printf("Zombie for PID -- %d failed.\n", done);
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Zombie for PID -- %d successfully removed.\n", done);
        }
    }
}