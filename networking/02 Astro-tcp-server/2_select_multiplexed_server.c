#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h> // Declare nonblocked socket
#include <sys/select.h>
#include <errno.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.
#define STATE_FINAL 0
#define STATE_RECEIVE_COMMAND 1
#define STATE_RECEIVE_HOROSCOPE 2
#define STATE_SENDING 3

#define COMMAN_LENGTH 22
#define BUFFER_LENGTH 80

// Task:
// Create a TCP-server which returns astrological forecasts as a response to client command HOROSCOPE.
// It should remember forecast on a STARS SAY command from client. Server should handle possible errors.
// This implementation should be full asynchronous based on "select" system call available in Linux.

// Compile:
// gcc -std=c99 2_select_multiplexed_server.c -o select_multiplexed_server

char zodiaks[12][12] = { "Aries      ", "Taurus     ", "Gemini     ", "Cancer     ",
"Leo        ", "Virgo      ", "Libra      ", "Scorpio    ", "Sagittarius",
"Capricorn  ", "Aquarius   ", "Pisces     " };

char descriptions[12][80] = { "", "", "", "", "", "", "", "", "", "", "", "" };

struct server_state
{
    int fd;
    int state;
    char buffer[BUFFER_LENGTH];
    char *position_in_buffer;
    ssize_t remain;
    int desc_index;
};

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

void init_for_receiving(struct server_state *s, ssize_t length)
{
    memset(s->buffer, '\0', BUFFER_LENGTH); // Clear before use.
    s->position_in_buffer = s->buffer;
    s->remain = length;
}

ssize_t receive_data(struct server_state *s)
{
    ssize_t received = recv(s->fd, s->position_in_buffer, s->remain, MSG_NOSIGNAL);

    if (received <= 0)
    {
        s->state = STATE_FINAL;
    }
    else if (received > 0)
    {
        s->remain -= received;
        s->position_in_buffer += received;
    }

    printf("Received from client: %d\n", received);

    return received;
}

void init_for_sending(struct server_state *s, const char *text, size_t text_len)
{
    memset(s->buffer, '\0', BUFFER_LENGTH); // Clear before use.
    memcpy(s->buffer, text, text_len); // Fill.
    s->position_in_buffer = s->buffer; // Set state.
    s->remain = strlen(s->buffer) + 1;
    s->state = STATE_SENDING;
}

void handle_command(struct server_state *s)
{
    printf("handle_command\n");
    int received = receive_data(s);

    if (s->remain > 0 && received > 0)
    {
        return; // need receive_data again
    }
    
    // Get znak zodiak
    char zodiak[11];
    for (int i = 10; i < 21; i++)
    {
        zodiak[i - 10] = s->buffer[i];
    }

    // Find it in array.
    s->desc_index = -1;
    for (int i = 0; i < 12; i++)
    {
        if (strncmp(zodiaks[i], zodiak, 11) == 0)
        {
            s->desc_index = i;
            break;
        }
    }
    
    if (s->desc_index == -1)
    {
        printf("Error in parsing client request: not found zodiak '%s'\n", zodiak);
        init_for_sending(s, "SORRY!", 7);
        return;
    }

    if (strncmp(s->buffer, "STARS SAY ", 10) == 0)
    {
        printf("Client request parsed: STARS SAY, '%s', position in array %d\n", zodiak, s->desc_index);
        s->state = STATE_RECEIVE_HOROSCOPE;
        init_for_receiving(s, BUFFER_LENGTH);
    }
    else if (strncmp(s->buffer, "HOROSCOPE ", 10) == 0)
    {
        printf("Client request parsed: HOROSCOPE, '%s', position in array %d\n", zodiak, s->desc_index);
        init_for_sending(s, descriptions[s->desc_index], sizeof(descriptions[s->desc_index]) + 1);
    }
    else
    {
        printf("Error in parsing client request: %s\n", zodiak);
        s->state = STATE_FINAL;
    }
}

void get_horoscope(struct server_state *s)
{
    printf("get_horoscope\n");
    int received = receive_data(s);

    if (s->remain == 0)
    {
        printf("Received horocope, trying to put in %d position.\n", s->desc_index);

        // Received data, save it.
        if (s->desc_index > -1)
        {
            printf("Stored in array: index %d, %s.\n", s->desc_index, descriptions[s->desc_index]);
            memcpy(descriptions[s->desc_index], s->buffer, strlen(s->buffer) + 1);
        }

        // Send thanks
        init_for_sending(s, "THANKS!", 8);
    }
}

void send_and_close(struct server_state *s)
{                    
    int sent = send(s->fd, s->position_in_buffer, s->remain, MSG_NOSIGNAL);
    printf("Sent to client: %d bytes\n", sent);

    if (sent <= 0)
    {
        s->state = STATE_FINAL;
        printf("send_and_close sent 0 -> STATE_FINAL\n");
        return;
    }
    else
    {
        s->remain -= sent;
        s->position_in_buffer += sent;
        if (s->remain == 0)
        {
            printf("send_and_close: STATE_FINAL\n");
            s->state = STATE_FINAL; // Close this connection, no need to wait another (STATE_RECEIVE_COMMAND). This is my choice.
            return;
        }
        // else Continue sending.
        printf("send_and_close: continue...\n");
    }
}

void die(const char *msg)
{
    perror(msg);
    exit(1); // TODO EXIT_FAILURE
}

////////////////////////////////////////////////////////////////////////////////
void main()
{
    printf("ASTRO Server PID -- %d.\n", getpid());
    
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
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);    
    serv_addr.sin_port = htons((int)PORTNUM); // Change bytes order to network'

    // Link socket with address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        die("Error of calling bind");
    }

    set_nonblock_mode(sockfd);
    
    // Server is ready to get SOMAXCONN connection requests (128).
    // This is *SHOULD* be enought to call accept and create child process.
    if (listen(sockfd, SOMAXCONN) == -1)
    {
        die("Error of calling listen");
    }
    
    fprintf(stderr, "Server is ready: %s\n", inet_ntoa(serv_addr.sin_addr));

    fd_set read_flags, write_flags; // the flag sets to be used in select

    //FD_SET(STDIN_FILENO, &read_flags);
   // FD_SET(STDIN_FILENO, &write_flags);
   
    // struct timeval waitd = {10, 0}; // the max wait time for an event
    
    int newsocket = -1;
    int max_fd = sockfd;
    int ready_fd = -1;
    struct server_state states_array[1024];
    
    for (int i = 0; i < 1024; i++)
    {
        states_array[i].fd = -1;
        states_array[i].state = STATE_FINAL;
        states_array[i].remain = 0;
        states_array[i].desc_index = -1;
    }
    
    int next = 0;
    
    while (1)
    {
        FD_ZERO(&read_flags);
        FD_ZERO(&write_flags);
        for (int i = 0; i < 1024; i++)
        {
            switch (states_array[i].state)
            {
            case STATE_RECEIVE_COMMAND:
            case STATE_RECEIVE_HOROSCOPE:
                FD_SET(states_array[i].fd, &read_flags);
                break;
            case STATE_SENDING:
                FD_SET(states_array[i].fd, &write_flags);
                break;
            case STATE_FINAL:
                {
                    if (states_array[i].fd != -1)
                    {
                        close(states_array[i].fd);
                        printf("Connection to fd %d closed!\n", states_array[i].fd);
                        states_array[i].fd = -1;
                    }
                }
                break;
            default:
                break;
            };
        }
        
        FD_SET(sockfd, &read_flags);
        
        printf("Select...\n");
        ready_fd = select(max_fd+1, &read_flags, &write_flags, (fd_set*)0, NULL); // &waitd);
        if(ready_fd == -1)
        {
            // What to do?
            perror("Error of calling select");
            continue;
            // exit(1);
        }
        else
        {
            printf("Number of ready descriptors: %d\n", ready_fd);
        }
                
        if (FD_ISSET(sockfd, &read_flags))
        {
            --ready_fd;
            
            printf("Trying to accept() new connection(s)\n");
            
            struct sockaddr_in clnt_addr;
            socklen_t addrlen = sizeof(clnt_addr);
            memset(&clnt_addr, '\0', sizeof(clnt_addr));
            if ((newsocket = accept(sockfd, (struct sockaddr *)&clnt_addr, &addrlen)) == -1)
            {
                if (errno != EWOULDBLOCK || errno != EAGAIN)
                    die("accept()");
            }
            
            if (newsocket != -1)
            {
                set_nonblock_mode(newsocket);
                
                memset(&states_array[next], 0, sizeof(states_array[next]));
                
                init_for_receiving(&states_array[next], COMMAN_LENGTH);
                states_array[next].state = STATE_RECEIVE_COMMAND;
                states_array[next].fd = newsocket;
                ++next;
                
                printf("Client = %s \n", inet_ntoa(clnt_addr.sin_addr));
                
                if (max_fd < newsocket)
                    max_fd = newsocket;
            }
        }        
        
        for (int i = 0; i < 1024 && ready_fd > 0; i++)
        {
            if (states_array[i].fd == -1 || states_array[i].state == STATE_FINAL)
                continue;
            
            if (FD_ISSET(states_array[i].fd, &read_flags))
            {
                --ready_fd;
                if (states_array[i].state == STATE_RECEIVE_COMMAND)
                {
                    handle_command(&states_array[i]);
                }
                else if (states_array[i].state == STATE_RECEIVE_HOROSCOPE)
                {
                    get_horoscope(&states_array[i]);
                }
                FD_CLR(states_array[i].fd, &read_flags);
            }
            
            if (FD_ISSET(states_array[i].fd, &write_flags))
            {
                --ready_fd;
                send_and_close(&states_array[i]);
                FD_CLR(states_array[i].fd, &write_flags);
            }
        }
        
        /*
        //if stdin is ready to be read
        //else if(FD_ISSET(STDIN_FILENO, &read_flags))
        //    fgets(out, 255, stdin);
        //socket ready for writing
        else if(FD_ISSET(newsocket, &write_flags)) {
            //printf("\nSocket ready for write");
            FD_CLR(newsocket, &write_flags);
            //send(newsocket, out, 255, 0);
            //memset(&out, 0, 255);
        }
        */
    }
    
    printf("Server stopped!");
    close(sockfd);
    exit(0);
}