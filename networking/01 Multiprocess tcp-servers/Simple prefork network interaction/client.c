#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h> // For compatibility
#include <sys/socket.h>
#include <arpa/inet.h> // For convertions htonl, htons, ntohl, ntohs
#include <netinet/in.h> // For sockaddr_in
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h> // For close()
//#include <errno> - don't use it because sometimes it may contains an error from another process

#define EXIT_FAILURE 1
#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

void main(int argc, char *argv[])
{
    struct hostent *hp;
    if ((hp = gethostbyname(argv[1])) == 0)
    {
        perror("Error of calling gethostbyname"); /* or use strerror */
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr)); // depricated function! TODO use memset
    bcopy(hp->h_addr_list[0], &serv_addr.sin_addr, hp->h_length); // depricated function! TODO use memcpy, memmove
    serv_addr.sin_family = hp->h_addrtype;
    serv_addr.sin_port = htons(PORTNUM);

    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Server address: %s\n", inet_ntoa(serv_addr.sin_addr));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Error of calling connect");
        exit(EXIT_FAILURE);
    }

    char buf[80] = "Hello, world!";
    send(sockfd, buf, sizeof(buf), MSG_NOSIGNAL);

    if (recv(sockfd, buf, sizeof(buf), MSG_NOSIGNAL) < 0)
    {
        perror("Error of calling recv");
        exit(EXIT_FAILURE);
    }

    printf("Received from server: %s\n", buf);

    // If fork was called then descriptor will be available in both parent- and child-processes.
    // Connection is closed only after closing both descriptions linked to socket.	
    close(sockfd);
    printf("Client off!\n\n");
}