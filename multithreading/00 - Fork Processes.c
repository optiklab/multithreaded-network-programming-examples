#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/wait.h> // For wait, waitpid

#define PORTNUM 1500  // Port > 1024 because program will not work not as root.
#define NUMBER_OF_PROCESSES 2
#define EXIT_FAILURE 1

// Compilation:
// gcc -std=gnu99 "00 - Fork Processes.c" -o forkProcesses

// Task:
// Use fork to create a child process.

int main(int argc, char *argv[])
{
    printf("Enter to main!\n");
 	for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
	{
		pid_t pid;
		int status;
		switch(pid=fork())
		{
			case -1:
                  printf("PROC %d:", i);
                  perror("1 Error of calling fork");
				  exit(EXIT_FAILURE);
			case 0:
				  printf("PROC %d: 1 CHILD: Child process %d!\n", i, getpid());
				  printf("PROC %d: 2 CHILD: Parent PID %d!\n", i, getppid());
				  printf("PROC %d: 3 CHILD: Wait 10 seconds...\n", i);
                  sleep(10);
                  printf("PROC %d: 4 CHILD: Exit!\n", i);
				  exit(EXIT_FAILURE);
			default:
				  printf("PROC %d: 1 PARENT: Parent process %d!\n", i, getpid());
				  printf("PROC %d: 2 PARENT: Child PID %d\n", i, pid);
				  printf("PROC %d: 3 PARENT: Wait until child calls exit()...\n", i);
				  waitpid(pid, &status, 0);
				  printf("PROC %d: 4 PARENT: Child exit code: %d\n", i, WEXITSTATUS(status));
				  printf("PROC %d: 5 PARENT: Exit!\n", i);
		}
	}

    printf("Exit main!\n");
    exit(0);
}