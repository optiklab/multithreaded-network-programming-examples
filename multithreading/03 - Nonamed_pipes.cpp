#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

struct CmdLine
{
    char* value;
    char* arguments;
    CmdLine* next;
};

// Compilation:
// g++ "03 - Nonamed_pipes.cpp" -o nonamedpipes

// Task:
// Create a program which works just like the command below:
// $> who | sort | uniq -c | sort -nk1

void run(CmdLine* command)
{
    const int numPipes = 3;
    int i = 0;
    pid_t pid;

    // Create all pipes.
    int pipefds[2*numPipes];
    for(i = 0; i < (numPipes); i++)
    {
        if(pipe(pipefds + i*2) < 0)
        {
            printf("Couldn't create pipe %d\n", i + 1);
            exit(EXIT_FAILURE);
        }
    }
    
    int j = 0;
    while(command)
    {
        pid = fork();
        if(pid == 0)
        {
            //if not last command
            if(command->next)
            {
                if(dup2(pipefds[j + 1], STDOUT_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            //if not first command && j != 2*numPipes
            if(j != 0 )
            {
                if(dup2(pipefds[j-2], STDIN_FILENO) < 0)
                {
                    perror(" dup2");///j-2 0 j+1 1
                    exit(EXIT_FAILURE);
                }
            }


            for(i = 0; i < 2*numPipes; i++)
            {
                close(pipefds[i]);
            }
            
            // Command with or without arguments.
            if (command->arguments == NULL)
            {
                if( execlp(command->value, command->value, NULL) < 0 )
                {
                    perror(command->value);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if( execlp(command->value, command->value, command->arguments, NULL) < 0 )
                {
                    perror(command->value);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if(pid < 0)
        {
            perror("error");
            exit(EXIT_FAILURE);
        }

        command = command->next;
        j+=2;
    }
    
    /* Parent closes the pipes and wait for children */
    for(i = 0; i < 2 * numPipes; i++)
    {
        close(pipefds[i]);
    }

    int status;
    for(i = 0; i < numPipes + 1; i++)
    {
        wait(&status);
    }
}

int main(int argc, char** argv)
{
    printf("Result of operation is the same like if you type\n$>who | sort | uniq -c | sort -nk1\n");
    
    char sort[] = "sort";
    char sortArg[] = "-nk1";
    char uniq[] = "uniq";
    char uniqArg[] = "-c";
    char who[] = "who";
    
    CmdLine command4 = { sort, sortArg, NULL };
    CmdLine command3 = { uniq, uniqArg, &command4 };
    CmdLine command2 = { sort, NULL, &command3 };
    CmdLine command1 = { who, NULL, &command2 };
    
    run(&command1);
    
    return 0;
}