#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h> // для совместимости
#include <sys/socket.h>
#include <arpa/inet.h> // Для преобразования htonl, htons, ntohl, ntohs
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h> // Для close()
//#include <errno> - не используем, иногда может содержать ошибку другого процесса

#define EXIT_FAILURE 1
#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

void main()
{
    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket"); /* или strerror */
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
    bzero(&serv_addr, sizeof(serv_addr)); // устаревшая, TODO use memset
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0

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

    struct sockaddr_in clnt_addr;
    int ns, pid;

    while (1)
    {
        socklen_t addrlen;

        bzero(&clnt_addr, sizeof(clnt_addr)); // устаревшая, TODO use memset
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

            char buf[80];
            while ((nbytes = recv(ns, buf, sizeof(buf), MSG_NOSIGNAL)) != 0)
            {
                send(ns, buf, sizeof(buf), 0);
            }

            close(ns);
            exit(0);
        }

        // Если был вызыван fork, то дескриптор будет доступен и в процессе родителя и child-процессе.
        // Соединение разрывается только когда закрыты все дескрипторы, связанные с сокетом.
        close(ns);

        printf("Server off! To change this behavior, look in code!\n\n");
        // To change this behavior just comment eixt(0) below:
        exit(0);
    }
}