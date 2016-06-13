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
// Create a prefork TCP server which sends «Hi there!\n» to client 5 times with 1 second time shift
// Client: $> nc 127.0.0.1 1500

#define EXIT_FAILURE 1
#define PORTNUM 1500 // Port > 1024 because program will not work not as root.
#define NUMBER_OF_PROCESSES 2

void main()
{
    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket"); /* or use strerror */
        exit(EXIT_FAILURE);
    }

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

    // Change bytes order to network'
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
    if (listen(sockfd, SOMAXCONN) == -1)
    {
        perror("Error of calling listen");
        exit(EXIT_FAILURE);
    }

    pid_t childs[NUMBER_OF_PROCESSES];
    struct sockaddr_in clnt_addr;
    int ns;

    // Create and handle child processes.
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        pid_t pid;
        int status;
        switch (pid = fork())
        {
        case -1:
            perror("Error of calling fork");
            exit(EXIT_FAILURE);  
        case 0:
        {
            int cpid = getpid();
            printf("NEW CHILD: PID -- %d, Parent PID -- %d\n", cpid, getppid());

            socklen_t addrlen;
            bzero(&clnt_addr, sizeof(clnt_addr));
            addrlen = sizeof(clnt_addr);


            printf("CHILD %d: Waiting to accept connection...\n", cpid);
            if ((ns = accept(sockfd, (struct sockaddr *)&clnt_addr, &addrlen)) == -1)
            {
                perror("Error of calling accept");
                exit(EXIT_FAILURE);
            }

            fprintf(stderr, "CHILD %d: Client = %s \n", cpid, inet_ntoa(clnt_addr.sin_addr));

            int counter = 0;
            char buf[11] = "Hi there!\n";
            while (counter != 5)
            {
                send(ns, buf, sizeof(buf), MSG_NOSIGNAL);
                ++counter;
                sleep(1);
            }

            // Descriptor continues to be available in parent process after fork() call.
            // We need to close all descriptors linked to socket.
            close(ns);
            printf("CHILD %d: Exit!\n", cpid);
            exit(0);
        }
        default:
            printf("PARENT: PID -- %d, remember NEW CHILD -- %d\n", getpid(), pid);
            childs[i] = pid;
            break;
        }
    }

    // Wait for ending child processes.
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++)
    {
        int status;
        printf("PARENT: Waiting while CHILD %d calls exit()...\n", childs[i]);
        while (-1 == waitpid(childs[i], &status, 0));
        printf("PARENT: Exit code of CHILD %d is: %d\n", childs[i], WEXITSTATUS(status));

        // If the child process fails to exit normally with a status of 0, then it didn't complete its work.
        // If the child processes are supposed to be killed by signals, or different exit codes are used, need to change this check accordingly.
        short isNormalTermination = WIFEXITED(status);
        if (!isNormalTermination ||
            // WEXITSTATUS should be used only if normal termination = true.
            (isNormalTermination && WEXITSTATUS(status) != 0))
        {
            printf("PARENT: CHILD %d failed...\n", childs[i]);
            exit(0);
        }
    }

    close(sockfd);
    exit(0);
}