#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h> // For compatibility
#include <sys/socket.h>
#include <arpa/inet.h> // For convertions htonl, htons, ntohl, ntohs
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h> // For close()
#include <sys/wait.h> // For wait, waitpid
//#include <errno> - don't use it because sometimes it may contains an error from another process

// Task:
// Create a process-per-request TCP server which sends «Hi there!\n» to client 5 times with 1 second time shift
// Client: $> nc 127.0.0.1 1500

#define EXIT_FAILURE 1
#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Most conforming applications that catch SIGCHLD are expected to install signal-catching functions 
// that repeatedly call the waitpid() function with the WNOHANG flag set, acting on each child for 
// which status is returned, until waitpid() returns zero. If stopped children are not of interest, 
// the use of the SA_NOCLDSTOP flag can prevent the overhead from invoking the signal-catching routine when they stop.
void child_zombie_handler(int sig)
{
    while (1)
    {
        int status;
        pid_t done = wait(&status);
        if (done == -1)
        {
            printf("No more child processes.\n", done);
            break;
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
}

void main()
{
    printf("Server PID -- %d.\n", getpid());
    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket"); /* or use strerror */
        exit(EXIT_FAILURE);
    }

    // For UDP SO_REUSEADDR may mean some problems...
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        perror("Error of calling setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set address
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 (INADDR_LOOPBACK - 127.0.0.1)

    // Change bytes order to network
    serv_addr.sin_port = htons((int)PORTNUM);

    // Link socket with address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Error of calling bind");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Server is ready: %s\n", inet_ntoa(serv_addr.sin_addr));

    // Server is ready to get SOMAXCONN connection requests (128).
    // This is *SHOULD* be enought to call accept and create child process.
    if (listen(sockfd, 128) == -1)
    {
        perror("Error of calling listen");
        exit(EXIT_FAILURE);
    }

    // Core notifies parent process about Zombie appearing by signal SIGCHLD.
    struct sigaction act;
    act.sa_handler = child_zombie_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &act, 0) == -1)
    {
        perror("Error of calling sigaction");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in clnt_addr;
    int ns, pid;

    int counter = 3;

    while (counter>1)
    {
        socklen_t addrlen;

        bzero(&clnt_addr, sizeof(clnt_addr));
        addrlen = sizeof(clnt_addr);

        if ((ns = accept(sockfd, (struct sockaddr *)&clnt_addr, &addrlen)) == -1)
        {
            perror("Error of calling accept");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Client = %s \n", inet_ntoa(clnt_addr.sin_addr));

        if ((pid = fork()) == -1)
        {
            perror("Error of calling fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            int nbytes;
            int fout;

            close(sockfd);

            int counter = 0;
            char buf[11] = "Hi there!\n";
            while (counter != 5)
            {
                send(ns, buf, sizeof(buf), MSG_NOSIGNAL);
                ++counter;
                sleep(1);
            }

            close(ns);
            exit(0);
            // CHILD process is zombie now. We call wait by SIGCHLD.
        }

        // If fork was called then descriptor will be available in both parent- and child-processes.
        // Connection is closed only after closing both descriptions linked to socket.
        close(ns);

        --counter;
        printf("Counter %d.\n", counter);
    }

    exit(0);
}