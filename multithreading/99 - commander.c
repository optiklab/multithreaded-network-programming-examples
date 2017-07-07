#include <fcntl.h>
#include <unistd.h> 
#include <sys/wait.h> // For wait, waitpid

#include "Common.h"

// Compilation:
// gcc -std=gnu99 ErrorHandling.c LogF.c "99 - commander.c" -o commander

// Task:
// Command interface - built from sources of book "Programming Unix".

#define MAXLINE 200
#define MAXARG 20

extern char **environ;

void set(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Too much arguments\n");
    }
    else
    {
        for (int i = 0; environ[i] != NULL; i++)
        {
            printf("%s\n", environ[i]);
        }
    }
}

void asg(int argc, char *argv[])
{
    char *name, *val;

    if (argc != 1)
    {
        printf("Too much arguments\n");
    }
    else
    {
        name = strtok(argv[0], "=");
        val = strtok(NULL, ""); /* Get right part of string */
        if (name == NULL || val == NULL)
        {
            printf("Incorrect command!\n");
        }
        else
        {
            ec_neg1(setenv(name, val, true));
        }
    }
EC_CLEANUP_BGN
    EC_FLUSH("asg")
EC_CLEANUP_END
}

// Returns True if string was parsed and tokenized correctly. Otherwise - False.
static bool getargs(int *argcp, char *argv[], int max, bool *eofp)
{
    static char cmd[MAXLINE];
    *eofp = false;

    // Read full string (200 chars).
    if (fgets(cmd, sizeof(cmd), stdin) == NULL) 
    {
        if (ferror(stdin))
        {
            EC_FAIL
        }
        *eofp = true;
        return false;
    }
     // In case we can't find end of string
    if (strchr(cmd, '\n') == NULL)
    {
        /* before showing an error we SHOULD eat last part of string until its end */
        while(true)
        {
            switch(getchar())
            {
                case '\n':
                    break;
                case EOF:
                    if (ferror(stdin))
                        EC_FAIL
                    break;
                default:
                    continue;
            }
        }
        
        printf("Command execution fail - too long string\r\n");
        return false;
    }

    int i = 0;
    char *cmdp = cmd;
    for (i = 0; i < max; i++)
    {
        // Split string to tokens.
        if ((argv[i] = strtok(cmdp, " \t\n")) == NULL)
        {
            break;
        }
        cmdp = NULL; /* for strtok - continue to work with that string */
    }
    if (i >= max)
    {
        printf("Command execution fail - too much arguments");
        return false;
    }
    *argcp = i;
    return true;
EC_CLEANUP_BGN
    EC_FLUSH("getargs")
    return false;
EC_CLEANUP_END
}

static void execute(int argc, char *argv[])
{
    pid_t pid;
    int status;

    switch (pid = fork())
    {
        case -1:
            EC_FAIL
        case 0:
            printf("CHILD: Child process %d!\n", getpid());
            printf("CHILD: Parent PID %d!\n", getppid());
            execvp(argv[0], argv);
        default:
            printf("PARENT: Parent process %d!\n", getpid());
            printf("PARENT: My child PID %d\n", pid);
            ec_neg1(waitpid(pid, &status, 0))
    }
EC_CLEANUP_BGN
    EC_FLUSH("execute")
    if (pid == 0)
        _exit(EXIT_FAILURE);
EC_CLEANUP_END
}

int main(void)
{
    char *argv[MAXARG];
    int argc;
    bool eof;
    while(true)
    {
       printf("@ ");
       if (getargs(&argc, argv, MAXARG, &eof) && argc > 0)
       {
           if (strchr(argv[0], '=') != NULL)
           {
               asg(argc, argv);
           }
           else if(strcmp(argv[0], "set")==0)
           {
               set(argc, argv);
           }
           else
           {
               execute(argc, argv);
           }
       }
       if (eof)
           exit(EXIT_SUCCESS);
    }
}