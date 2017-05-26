#include <stdio.h>
#include <stdlib.h>
#include "Common.h"

extern char ** environ; // Global variable availble to any process for read/write.

#define ENVVARSAPP "ENVVARSAPP=test"

#define OVERWRITE 1

// Compilation:
// gcc -std=gnu99 "01 - Env Vars.c" -o envVars

// Task:
// Read, write environment variables.

int main(int argc, char *argv[])
{
	printf("Variables available process:\n");
	for (int i = 0; environ[i] != NULL; i++)		
	{
		printf("%s\n", environ[i]);
	}
	
	// Get HOME variable
	char *s = getenv("HOME");
	if (s == NULL)
	{
		printf("HOME not found.\n");
	}
	else
	{
		printf("Value of HOME: \"%s\"\n", s);
	}
	
	// Put new variable.
	// WARNING! Don't put local or automatic non-static variables since it will be used by system.
	if (putenv(ENVVARSAPP) == FAILURE) 
	{
		perror("putenv ENVVARSAPP failed!\n");
	}
	else
	{
		printf("putenv ENVVARSAPP succeeded!\n");
	}
	
	// Set new variable.
	if (setenv("ENVVARSAPP2", "test2", OVERWRITE) == FAILURE) 
	{
		perror("setenv ENVVARSAPP2 failed!\n");
	}
	else
	{
		printf("setenv ENVVARSAPP2 succeeded!\n");
	}
	
	// Remove variables.
	if (unsetenv("ENVVARSAPP") == FAILURE) 
	{
		perror("unsetenv ENVVARSAPP failed!\n");
	}
	else
	{
		printf("unsetenv ENVVARSAPP succeeded!\n");
	}
	
	if (unsetenv("ENVVARSAPP2") == FAILURE) 
	{
		perror("unsetenv ENVVARSAPP2 failed!\n");
	}
	else
	{
		printf("unsetenv ENVVARSAPP2 succeeded!\n");
	}	
	
    printf("Exit process!\n");
    exit(EXIT_SUCCESS);
}