#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <ev.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.

// Install libev-dev via Synaptic Package Manager

// Compile
// gcc -lev server.c -o server

// Run client:
// telnet 127.0.0.1

void die(const char *msg)
{
    perror(msg);
    exit(1); // TODO EXIT_FAILURE
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    char buffer[1024];
    ssize_t r = recv(watcher->fd, buffer, 1024, MSG_NOSIGNAL);
    
    if(r<0)
    {
        printf("Server received <0, so do nothing.\n");
        return;
    }
    else if (r == 0)
    {
        printf("Server received 0, so finishing connection with client...\n");
        ev_io_stop(loop, watcher);
        free(watcher);
        return;
    }
    else
    {
        printf("Server received >0, so sending...\n");
        send(watcher->fd, buffer, r, MSG_NOSIGNAL);
    }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    int client_sock_fd = accept(watcher->fd, 0, 0);
    
    struct ev_io *w_client = (struct ev_io*)malloc(sizeof(struct ev_io));
    
    printf("Starting Loop after accept.\n");
    
    ev_io_init(w_client, read_cb, client_sock_fd, EV_READ);
    ev_io_start(loop, w_client);
    
    printf("Server accepted client to socket %d\n", client_sock_fd);
}

int main(int argc, char **argv)
{
    printf("Server based on LibEv PID -- %d.\n", getpid());

    struct ev_loop *loop = ev_default_loop(0);
    
    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        die("Error of calling socket"); /* или strerror */
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
    
    bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    listen(sockfd, SOMAXCONN);
    
    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, sockfd, EV_READ);
    ev_io_start(loop, &w_accept);
    
    while(1) ev_loop(loop, 0);
    
    return 0;
}