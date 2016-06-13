#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int MasterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

    bind(MasterSocket, (struct sockaddr *)(&SockAddr), sizeof(SockAddr));

    listen(MasterSocket, SOMAXCONN);

    while (1)
    {
        int SlaveSocket = accept(MasterSocket, 0, 0);
        int Buffer[5] = { 0, 0, 0, 0, 0 };
        recv(SlaveSocket, Buffer, 4, MSG_NOSIGNAL);
        shutdown(SlaveSocket, SHUT_RDWR);
        close(SlaveSocket);
        printf("%s\n", Buffer);
    }

    return 0;
}