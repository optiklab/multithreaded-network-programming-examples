#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Common.h"

// Compilation:
// gcc "05 - Socketpair_fd_passing.c" -o socketpairfdpassing

// Task:
// Use of FD-passing technology to let 2 processes/programs communicate with each other and send sockets.
// This technology should be helpful in programming multiprocess server programming to handle socket connections
// in child processes instead of single parent process.


// Send information in a buffer via socket and attached additional information with that buffer i.e. our file descriptor.
// We may send several file descriptors, but this is a simple example.
ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd)
{
    // We should send at least 1 byte of any information except file descriptor itself.
    if (buflen < 0)
    {
        return -1;
    }

    // IOVector - describes information buffer.
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = buflen;

    struct msghdr msg;
    //  Send only one buffer IOVector, however sendmsg() may work with several buffers by merging them together.
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1)
    {
        // Declare it here because it may be absent in headers.
        union
        {
            struct cmsghdr  cmsghdr;
            char            control[CMSG_SPACE(sizeof(int))];
        } cmsgu;

        // System info.
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        
        // Use constans and macroses to fill info.
        struct cmsghdr *cmsg;
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        printf("passing fd %d\n", fd);
        *((int *)CMSG_DATA(cmsg)) = fd;
    }
    else
    {
        // We don't need to send file descriptor - work in usual sending mode.
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        printf("not passing fd\n");
    }

    // Sending data via sockets, similar to sendto.
    ssize_t size = sendmsg(sock, &msg, 0);
    if (size < 0)
        perror("sendmsg");
    
    return size;
}

ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize,   
    // Result file descriptor.
    int *fd)
{
    ssize_t     size;
    if (fd)
    {
        struct iovec    iov;
        iov.iov_base = buf;
        iov.iov_len = bufsize;
        
        struct msghdr   msg;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        
        union
        {
            struct cmsghdr  cmsghdr;
            char            control[CMSG_SPACE(sizeof(int))];
        } cmsgu;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        
        size = recvmsg(sock, &msg, 0);
        
        if (size < 0)
        {
            perror("recvmsg");
            exit(EXIT_FAILURE);
        }

        struct cmsghdr  *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
        {
            if (cmsg->cmsg_level != SOL_SOCKET)
            {
                fprintf(stderr, "invalid cmsg_level %d\n", cmsg->cmsg_level);
                exit(EXIT_FAILURE);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS)
            {
                fprintf(stderr, "invalid cmsg_type %d\n", cmsg->cmsg_type);
                exit(EXIT_FAILURE);
            }
            *fd = *((int *)CMSG_DATA(cmsg));
            printf("received fd %d\n", *fd);
        }
        else
            *fd = -1;
    }
    else
    {
        size = read(sock, buf, bufsize);
        if (size < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    
    return size;
}

void child_read(int sock)
{
    int fd;
    char buf[16];
    ssize_t size;
    sleep(1);
    
    size = sock_fd_read(sock, buf, sizeof(buf), &fd);
    if (size > 0)
    {
        printf ("read size: %d, fd: %d, buffer: %s\n", (int)size, fd, buf);
    }
}

void parent_writes(int sock)
{
    ssize_t size = sock_fd_write(sock, "TEST", 4, 1);
    printf ("wrote size: %d\n", (int)size);
}

/*
// Multiple writes were made, some with file descriptors and some without
void parent_writes(int sock)
{
    ssize_t size = sock_fd_write(sock, "1", 1, -1);
    printf ("wrote %d without fd\n", size);
    size = sock_fd_write(sock, "1", 1, 1);
    printf ("wrote %d with fd\n", size);
    size = sock_fd_write(sock, "1", 1, -1);
    printf ("wrote %d without fd\n", size);
}
*/

/*

// This shows that the first passed file descriptor is picked up by the first sock_fd_read call, but the file descriptor is closed.
// The second file descriptor passed is picked up by the second sock_fd_read call.
void child_read(int sock)
{
    int fd;
    char    buf[16];
    ssize_t size;
    sleep(1);
    size = sock_fd_read(sock, buf, sizeof(buf), NULL);
    if (size <= 0)
        return;
    printf ("read %d\n", size);
    size = sock_fd_read(sock, buf, sizeof(buf), &fd);
    if (size <= 0)
        return;
    printf ("read %d\n", size);
    if (fd != -1)
    {
        write(fd, "hello, world\n", 13);
        close(fd);
    }
}

void parent_writes(int sock)
{
    ssize_t size = sock_fd_write(sock, "1", 1, 1);
    printf ("wrote %d without fd\n", size);
    size = sock_fd_write(sock, "1", 1, 2);
    printf ("wrote %d with fd\n", size);
}
*/

int main(int argc, char **argv)
{
    int sv[2];
    pid_t pid;
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0)
    {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }
    
    switch ((pid = fork()))
    {
    case 0:
        close(sv[0]);
        child_read(sv[1]);
        break;
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
    default:
        close(sv[1]);
        parent_writes(sv[0]);
        break;
    }
    
    int status;
    waitpid(pid, &status, 0);
    
    return 0;
}
