#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/wait.h> // For wait, waitpid

#include "Common.h"

// Compilation:
// gcc -std=gnu99 "03 - Process attributes.c" -o processAttrs

// Task:
// Work with process attributes.

int main(int argc, char *argv[])
{
    printf("Enter to main!\n");
    printf("Create 1 child process...\n");
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
			printf("3 CHILD: Who created me? User ID: %d\n", getuid());
			printf("3 CHILD: Who created me? Effective User ID: %d\n", geteuid());
			printf("3 CHILD: Who created me? Group ID: %d\n", getgid());
			printf("3 CHILD: Who created me? Effective Group ID: %d\n", getegid());
			
			// WARNING!! Works only under ROOT.
			// Changing root from / to some another directory (Example: restrict access to directories more than Web Server directories for Security).
			//printf("4 CHILD: Let's change Working Dir: %d\n", chdir(""));
			//printf("4 CHILD: Let's change Working Dir: %d\n", fchdir(""));
			//printf("4 CHILD: Let's change Root Dir: %d\n", chroot(""));
			// Process priority is in range N=[0, 39]. Nice returns N-20. In case of error Nice returns -1. If N == 19 ir returns -1 TOO :).
			printf("5 CHILD: Let's decrease Priority Level for N=5: %d (N-20)\n", nice(5));
			printf("5 CHILD: Let's increase Priority Level for N=-5: %d (N-20)\n", nice(-5));
			
			printf("6 CHILD: Wait 4 seconds...\n");
			sleep(4);
			printf("7 CHILD: Exit!\n");
			exit(EXIT_FAILURE);
		default:
			printf("1 PARENT: Parent process %d!\n", getpid());
			printf("2 PARENT: Child PID %d\n", pid);
			printf("3 PARENT: Who created me? User ID: %d\n", getuid());
			printf("3 PARENT: Who created me? Group ID: %d\n", getgid());
			printf("3 PARENT: Who created me? Effective (under which permissions) User ID: %d\n", geteuid());
			printf("3 PARENT: Who created me? Effective (under which permissions) Group ID: %d\n", getegid());
			waitpid(pid, &status, 0);
			printf("4 PARENT: Child exit code: %d\n", WEXITSTATUS(status));
			printf("5 PARENT: Exit!\n");
	}
	
    printf("Exit process!\n");
    exit(EXIT_SUCCESS);
}