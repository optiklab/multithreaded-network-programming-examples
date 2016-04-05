#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/wait.h> // For wait, waitpid

#define PORTNUM 1500  // Port > 1024 because program will not work not as root.
#define NUMBER_OF_PROCESSES 2
#define EXIT_FAILURE 1

// Compilation:
// gcc -std=gnu99 "00 - Processes.c" -o processes

// Task:
// Use fork to create a child process.

void main(int argc, char *argv[])
{
    printf("0 Enter to main!\n");
 	for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
	{
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
				  printf("3 CHILD: Wait 10 seconds...\n");
                  sleep(10);
                  printf("4 CHILD: Exit!\n");                  
				  exit(EXIT_FAILURE);
			default:
				  printf("1 PARENT: Parent process %d!\n", getpid());
				  printf("2 PARENT: Child PID %d\n", pid);
				  printf("3 PARENT: Wait until child calls exit()...\n");
				  waitpid(pid, &status, 0);
				  printf("4 PARENT: Child exit code: %d\n", WEXITSTATUS(status));
				  printf("5 PARENT: Exit!\n");
		}
	}

    printf("1 Exit main!\n");
    exit(0);
}