#include <iostream>
#include <set>
#include <algorithm>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Compile:
// g++ -std=c++11 3_kqueue_server.cpp -o kqueue_server

int set_nonblock_mode(int fd)
{
    int flags;
#if defined (O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    {
        flags = 0;
    }

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
}

void die(const char *msg)
{
    // Move latest errno to stderr with message msg.
    perror(msg);

    // perror - POSIX standard function.
    // or use strerror(errno);

    exit(1); // TODO EXIT_FAILURE
}

// Single-threaded echo server handles 1024 < clients in multiplexing mode with select.
// Use client: telnet 127.0.0.1 12345
// just type anything...
int main(int argc, char **argv)
{
    // Create socket
    int masterSocketFd = -1;
    if ((masterSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        die("Error of calling socket");
    }

    // For UDP SO_REUSEADDR may mean some problems...
    int optval = 1;
    if (setsockopt(masterSocketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        die("Error of calling setsockopt");
    }

    std::set<int> slaveSockets;
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    serverAddress.sin_port = htons((int)PORTNUM);

    // Link socket with address
    if (bind(masterSocketFd, (struct sockaddr *)(&serverAddress), sizeof(serverAddress)) == -1)
    {
        die("Error of calling bind");
    }

    set_nonblock_mode(masterSocketFd);

    // Server is ready to get SOMAXCONN connection requests (128).
    // This is *SHOULD* be enought to call accept and create child process.
    if (listen(masterSocketFd, SOMAXCONN) == -1)
    {
        die("Error of calling listen");
    }

    std::cout << "Server is ready: " << inet_ntoa(serverAddress.sin_addr) << std::endl;

    int kQueue = kqueue();
    
    struct kevent kEvent; // Here may be more than 1, for example 10 etc.
    bzero(&kEvent, sizeof(kEvent));
    EV_SET(&kEvent, masterSocketFd, EVFILT_READ, EV_ADD, 0, 0, 0);

    // Register event
    kevent(kQueue, &kEvent, 1,
        // We only read, not write, so set NULL, 0
        NULL, 0,
        NULL);

    while (true)
    {
        // Handle events 1 by 1. It is possible to do it in a batch.
        bzero(&kEvent, sizeof(kEvent));
        kevent(kQueue, NULL, 0, &kEvent, 1, NULL);
        
        if (kEvent.filter = EVFILT_READ)
        {
            if (kEvent.ident == masterSocketFd)
            {
                struct sockaddr_in clientAddress;
                socklen_t addressLength = sizeof(clientAddress);
                memset(&clientAddress, '\0', sizeof(clientAddress));
                int slaveSocket = -1;
                if ((slaveSocket = accept(masterSocketFd, (struct sockaddr *)&clientAddress, &addressLength)) == -1)
                {
                    if (errno != EWOULDBLOCK || errno != EAGAIN)
                        die("Error of calling accept");
                }

                set_nonblock_mode(slaveSocket);

                bzero(&kEvent, sizeof(kEvent));
                EV_SET(&kEvent, slaveSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
            }            
            else
            {
                static char Buffer[1024];
                int RecvSize = recv(events[i].data.fd, Buffer, 1024, MSG_NOSIGNAL);

                if (RecvSize <= 0)
                {
                    close(kEvent.ident);
                    printf("disconnected!\n");
                }
                else
                {
                    send(kEvent.ident, Buffer, RecvSize, MSG_NOSIGNAL);
                }
            }
        }
    }

    return 0;
}