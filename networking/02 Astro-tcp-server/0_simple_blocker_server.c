#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Task:
// Create a TCP-server which returns astrological forecasts as a response to client command HOROSCOPE.
// It should remember forecast on a STARS SAY command from client. Server should handle possible errors.

// Compile:
// gcc -std=c99 0_simple_blocker_server.c -o simple_blocker_server

// Function read number of bytes and return it. It is necessary to use such a function for read,
// because system call recv may return not full response and it will be necessary to read in loop.
ssize_t receive_all(int fd, char *buf, size_t len)
{
    size_t  pos = 0;

    while (pos < len)
    {
        // Buffer returned by system call recv - is not the string, so we can't use string functions with it.
        ssize_t received = recv(fd, buf + pos, len - pos, MSG_NOSIGNAL); // Receive byte by byte.

        if (received == -1) return -1;
        else if (received == 0) return pos;
        else if (received > 0) pos += received;
    }

    return pos;
}

/* 
// Just another example of the same receive algorithm.
// Taken from book Effective TCP/IP.
int receive_all(int fd, char *buf, size_t len)
{
    int cnt, rc;
    cnt = len;
    while (cnt > 0)
    {
        rc = recv(fd, buf, cnt, 0);
        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rc == 0)
            return len - cnt;
        buf += rc;
        cnt -= rc;
    }
}
*/

void main()
{
    printf("ASTRO Server PID -- %d.\n", getpid());

    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket"); /* or strerror */
        exit(1);
    }

    // For UDP SO_REUSEADDR may mean some problems...
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        perror("Error of calling setsockopt");
        exit(1);
    }

    // Set address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 (INADDR_LOOPBACK - 127.0.0.1)

    // Change bytes order to network'
    int nport = htons((int)PORTNUM);
    serv_addr.sin_port = nport;

    // Link socket with address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Error of calling bind");
        exit(1);
    }

    fprintf(stderr, "Server is ready: %s\n", inet_ntoa(serv_addr.sin_addr));

    // Server is ready to get SOMAXCONN connection requests (128).
    // This is *SHOULD* be enought to call accept and create child process.
    if (listen(sockfd, SOMAXCONN) == -1)
    {
        perror("Error of calling listen");
        exit(1);
    }

    struct sockaddr_in clnt_addr;
    socklen_t addrlen = sizeof(clnt_addr);
    memset(&clnt_addr, '\0', sizeof(clnt_addr));

    int newsocket = -1;

    char zodiaks[12][12] = { "Aries      ", "Taurus     ", "Gemini     ", "Cancer     ",
        "Leo        ", "Virgo      ", "Libra      ", "Scorpio    ", "Sagittarius",
        "Capricorn  ", "Aquarius   ", "Pisces     " };

    char descriptions[12][80] = { "", "", "", "", "", "", "", "", "", "", "", "" };

    char thanks[] = "THANKS!";
    char denied[] = "DENIED!";
    char sorry[] = "SORRY!";

    while (1)
    {
        if ((newsocket = accept(sockfd, (struct sockaddr *)&clnt_addr, &addrlen)) == -1)
        {
            perror("Error of calling accept");
            exit(1);
        }

        printf("Client = %s \n", inet_ntoa(clnt_addr.sin_addr));

        char command[22];
        memset(command, '\0', 22);
        int received = receive_all(newsocket, command, 22);

        printf("Received from client: %s, %d bytes\n", command, received);

        if (received == 22)
        {
            // Get zodiak
            char zodiak[11];
            for (int i = 10; i < 21; i++)
            {
                zodiak[i - 10] = command[i];
            }

            printf("Received from client: zodiak %s\n", zodiak);

            // Find it in array.
            int desc_index = -1;
            for (int i = 0; i < 12; i++)
            {
                if (strncmp(zodiaks[i], zodiak, 12) == 0)
                {
                    desc_index = i;
                    break;
                }
            }

            printf("Position in array of zodiaks: %d\n", desc_index);

            if (strncmp(command, "STARS SAY ", 10) == 0)
            {
                // If found, get description.
                if (desc_index > -1)
                {
                    // Receive 80 bytes of description.
                    char desc[80];
                    memset(desc, '\0', 80);

                    int received_dest = receive_all(newsocket, desc, 80);

                    if (received_dest > 0)
                    {
                        memcpy(descriptions[desc_index], desc, received_dest);
                    }

                    printf("Received from client: %s, %d bytes\n", desc, received_dest);

                    // Send thanks
                    int sent = send(newsocket, thanks, strlen(thanks) + 1, MSG_NOSIGNAL);
                    printf("Sent to client: %s, %d bytes\n", thanks, sent);
                }
                else
                {
                    int sent = send(newsocket, denied, strlen(denied) + 1, MSG_NOSIGNAL);
                    printf("Sent to client: %s, %d bytes\n", denied, sent);
                }
            }
            else if (strncmp(command, "HOROSCOPE ", 10) == 0)
            {
                // Send 80 bytes description
                if (desc_index > -1)
                {
                    if (strlen(descriptions[desc_index]) > 1)
                    {
                        int sent = send(newsocket, descriptions[desc_index], sizeof(descriptions[desc_index]), MSG_NOSIGNAL);
                        printf("Sent to client: %s, %d bytes\n", descriptions[desc_index], sent);
                    }
                    else
                    {
                        int sent = send(newsocket, sorry, strlen(sorry) + 1, MSG_NOSIGNAL);
                        printf("Sent to client: %s, %d bytes\n", sorry, sent);
                    }
                }
                else
                {
                    int sent = send(newsocket, denied, strlen(denied) + 1, MSG_NOSIGNAL);
                    printf("Sent to client: %s, %d bytes\n", denied, sent);
                }
            }
        }

        printf("Connection to client closed!");
        close(newsocket);
    }

    printf("Server stopped!");
    close(sockfd);
    exit(0);
}