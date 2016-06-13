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
#include <poll.h>

#define POLL_SIZE 2048 // Artifical limit for Poll since it is not limited to 1024 file descriptors like Select.

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Compile:
// g++ -std=c++11 1_poll_server.cpp -o poll_server

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
    
    struct pollfd descriptors[POLL_SIZE];
    descriptors[0].fd = masterSocketFd;
    descriptors[0].events = POLLIN;
    
    while (true)
    {
        unsigned int index = 1;
        
        for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); iter++)
        {
            descriptors[index].fd = *iter;
            descriptors[index].events = POLLIN;
            ++index;
        }

        unsigned int length = 1 + slaveSockets.size();        
        
        int readyDesc = poll(descriptors, length, -1);
        if(readyDesc == -1)
        {
            die("Error of calling poll");
        }
        else
        {
            std::cout << "Number of ready descriptors: " << readyDesc << std::endl;
        }
        
        for (unsigned int i = 0; i < length; i++)
        {
            if (descriptors[i].revents & POLLIN)
            {
                if (i > 0) // Slave socket
                {
                    static char Buffer[1024];
                    int RecvSize = recv(descriptors[i].fd, Buffer, 1024, MSG_NOSIGNAL);

                    if ((RecvSize == 0) && (errno != EAGAIN))
                    {
                        // If we got event TO READ, but actually CANNOT read, this means we should CLOSE connection. This is how POLL and EPOLL works. 
                        shutdown(descriptors[i].fd, SHUT_RDWR);
                        close(descriptors[i].fd);
                        slaveSockets.erase(descriptors[i].fd);
                    }
                    else if (RecvSize != 0)
                    {
                        send(descriptors[i].fd, Buffer, RecvSize, MSG_NOSIGNAL);
                    }
                }
                else // Master socket
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
        }
    }
    
    return 0;
}