#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Task:
// Create a test TCP-client which sends commands to TCP-server:
// HOROSCOPE - get astrological forecasts from server.
// STARS SAY - send forecast to server.

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

int main(int argc, char **argv)
{
    struct hostent *hp;
    if ((hp = gethostbyname(argv[1])) == 0)
    {
        perror("Error of calling gethostbyname"); /* Or use strerror */
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    bcopy(hp->h_addr_list[0], &serv_addr.sin_addr, hp->h_length);
    serv_addr.sin_family = hp->h_addrtype;
    serv_addr.sin_port = htons(PORTNUM);

    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error of calling socket");
        exit(1);
    }

    fprintf(stderr, "Server address: %s\n", inet_ntoa(serv_addr.sin_addr));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Error of calling connect");
        exit(1);
    }

    int testStep = (*argv[2]) - '0';
    printf("testStep: %d\n", testStep);

    if (testStep == 1)
    {
        // TEST1. Set some HOROSCOPE.

        char setzodiak[] = "STARS SAY Aries      ";
        int sent = send(sockfd, setzodiak, strlen(setzodiak) + 1, 0);
        printf("Sent to server: %s, %d bytes\n", setzodiak, sent);

        char desc[] = "Some of my descriptions here.                                                  ";
        sent = send(sockfd, desc, strlen(desc) + 1, 0);
        printf("Sent to server: %s, %d bytes\n", desc, sent);

        char thanks[10];
        memset(thanks, '\0', 10);
        int received = receive_all(sockfd, thanks, 10);
        printf("Received from server: %d bytes\n", received);
        if (received > 0)
        {
            printf("Received from server: %s\n", thanks);
        }
    }
    else if (testStep == 2)
    {
        // TEST2. Get HOROSCOPE which was set.
        char getzodiak[] = "HOROSCOPE Aries      ";
        int sent = send(sockfd, getzodiak, strlen(getzodiak) + 1, 0);
        printf("Sent to server: %s, %d bytes\n", getzodiak, sent);

        char horoscope[80];
        memset(horoscope, '\0', 80);
        int received = receive_all(sockfd, horoscope, 80);
        printf("Received from server: %d bytes\n", received);
        if (received > 0)
        {
            printf("Received from server: %s\n", horoscope);
        }
    }
    else if (testStep == 3)
    {
        // TEST3. Good COMMAND, Bad attributes.
        char getzodiak[] = "HOROSCOPE Abracadabra";
        int sent = send(sockfd, getzodiak, strlen(getzodiak) + 1, 0);
        printf("Sent to server: %s, %d bytes\n", getzodiak, sent);

        char denied[10];
        memset(denied, '\0', 10);
        int received = receive_all(sockfd, denied, 10);
        printf("Received from server: %d bytes\n", received);
        if (received > 0)
        {
            printf("Received from server: %s\n", denied);
        }
    }
    else if (testStep == 4)
    {
        // TEST4. Wrong COMMAND.
        char wrongCommand[] = "ABRA CADABRA BOOM BOOM";
        int sent = send(sockfd, wrongCommand, strlen(wrongCommand) + 1, 0);
        printf("Sent to server: %s, %d bytes\n", wrongCommand, sent);

        char denied[10];
        memset(denied, '\0', 10);
        int received = receive_all(sockfd, denied, 10);
        printf("Received from server: %d bytes\n", received);
    }

    close(sockfd);
    printf("Client off!\n\n"); \
}
