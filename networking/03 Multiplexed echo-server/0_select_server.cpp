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

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Compile:
// g++ -std=c++11 0_select_server.cpp -o select_server

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
    
    while (true)
    {
        fd_set readFlags, writeFlags;
        FD_ZERO(&readFlags);
        
        FD_SET(masterSocketFd, &readFlags);
        for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); iter++)
        {
            FD_SET(*iter, &readFlags);
        }

        int max = std::max(masterSocketFd,
            *std::max_element(slaveSockets.begin(), slaveSockets.end()));

        int readyDesc = select(max + 1, &readFlags, &writeFlags, (fd_set*)0, NULL);
        if(readyDesc == -1)
        {
            die("Error of calling select");
        }
        else
        {
            std::cout << "Number of ready descriptors: " << readyDesc << std::endl;
        }
        
        for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); iter++)
        {
            if (FD_ISSET(*iter, &readFlags))
            {
                static char Buffer[1024];
                int RecvSize = recv(*iter, Buffer, 1024, MSG_NOSIGNAL);

                if ((RecvSize == 0) && (errno != EAGAIN))
                {
                    shutdown(*iter, SHUT_RDWR);
                    close(*iter);
                    slaveSockets.erase(iter);
                }
                else if (RecvSize != 0)
                {
                    send(*iter, Buffer, RecvSize, MSG_NOSIGNAL);
                }
            }
        }

        if (FD_ISSET(masterSocketFd, &readFlags))
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
            
            slaveSockets.insert(slaveSocket);
        }
    }
    
    return 0;
}