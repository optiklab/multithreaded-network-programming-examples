#ifndef _DEFS_H_
#define _DEFS_H_

// For ErrorHandling
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h> // For wait, waitpid
#include <sys/resource.h> // For working with rlimits
#include <signal.h>

// For LogF
#include <unistd.h>
#include <fcntl.h>

#include <stdarg.h>
#include <time.h>
#include <limits.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define FAILURE -1

typedef int bool;
#define true 1
#define false 0

#include "ErrorHandling.h"
#include "LogF.h"

/*File-permission-bit symbols*/
#define PERM_DIRECTORY	S_IRWXU
#define PERM_FILE		(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
 
#endif /* _DEFS_H_ */