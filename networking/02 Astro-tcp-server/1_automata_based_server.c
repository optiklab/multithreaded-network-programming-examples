#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#define PORTNUM 1500 // Port > 1024 because program will not work not as root.
#define STATE_FINAL 0
#define STATE_SUCCESS 1
#define STATE_RECEIVE_COMMAND 2
#define STATE_RECEIVE_HOROSCOPE 3
#define STATE_SEND_THANKS 4
#define STATE_SEND_HOROSCOPE 5
#define STATE_SEND_DENINED 6
#define STATE_SEND_SORRY 7
#define STATE_SENDING 8
#define STATE_ERROR 9

#define ERROR -1
#define STARS_SAY 0
#define HOROSCOPE 1

#define COMMAN_LENGTH 22
#define BUFFER_LENGTH 80

// Task:
// Create a TCP-server which returns astrological forecasts as a response to client command HOROSCOPE.
// It should remember forecast on a STARS SAY command from client. Server should handle possible errors.
// This implementation should be created as automata - it is a middle step from simple blocked server to fully asynchronous server.

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

ssize_t receive_data(struct server_state *s)
{
    ssize_t received = recv(s->fd, s->position_in_buffer, s->remain, MSG_NOSIGNAL);

    if (received == -1)
    {
        s->state = STATE_FINAL;
    }
    else if (received == 0)
    {
        s->state = (s->remain == COMMAN_LENGTH) ? STATE_SUCCESS : STATE_ERROR;
    }
    else if (received > 0)
    {
        s->remain -= received;
        s->position_in_buffer += received;
    }

    printf("Received from client: %d\n", received);

    return received;
}

int parse_command(struct server_state *s)
{
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

    if (strncmp(s->buffer, "STARS SAY ", 10) == 0)
    {
        printf("Client request parsed: STARS SAY, %s, position in array %d\n", zodiak, s->desc_index);
        return STARS_SAY;
    }
    else if (strncmp(s->buffer, "HOROSCOPE ", 10) == 0)
    {
        printf("Client request parsed: HOROSCOPE, %s, position in array %d\n", zodiak, s->desc_index);
        return HOROSCOPE;
    }
    else
    {
        printf("Client request parsed: ERROR, %s, position in array %d\n", zodiak, s->desc_index);
        return ERROR;
    }
}

void get_command(struct server_state *s)
{
    int received = receive_data(s);

    if (s->remain > 0)
    {
        return;
    }

    // Received data, let's parse it.
    int result = parse_command(s);

    if (result == ERROR)
    {
        s->state = STATE_ERROR;
    }
    else if (result == STARS_SAY)
    {
        // s->desc_index impliciltly filled in parse_command.
        s->state = STATE_RECEIVE_HOROSCOPE;
    }
    else if (result == HOROSCOPE)
    {
        // s->desc_index impliciltly filled in parse_command.
        s->state = STATE_SEND_HOROSCOPE;
    }
}

void get_horoscope(struct server_state *s)
{
    int received = receive_data(s);

    if (s->remain == 0)
    {
        printf("Received horocope, trying to put in %d position.\n", s->desc_index);

        // Received data, save it.
        if (s->desc_index > -1)
        {
            printf("Stored in array: %s.\n", descriptions[s->desc_index]);
            memcpy(descriptions[s->desc_index], s->buffer, strlen(s->buffer) + 1);
        }

        // Send thanks
        s->state = STATE_SEND_THANKS;
    }
}

void send_and_close(struct server_state *s)
{
    int sent = send(s->fd, s->position_in_buffer, s->remain, MSG_NOSIGNAL);
    printf("Sent to client: %d bytes\n", sent);

    if (sent <= 0)
    {
        s->state = STATE_ERROR;
        printf("send_and_close: STATE_ERROR\n");
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

////////////////////////////////////////////////////////////////////////////////
void main()
{
    printf("ASTRO Server PID -- %d.\n", getpid());
    
    // Create socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Error of calling socket"); /* или strerror */
        exit(1);
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

    // Link socket with address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Error of calling bind");
        exit(1);
    }

    fprintf(stderr, "Server is ready: %s\n", inet_ntoa(serv_addr.sin_addr));

    // Server is ready to get SOMAXCONN connection requests (128).
    // This is *SHOULD* be enought to call accept and create child process.
    if (listen(sockfd, SOMAXCONN) == -1)
    {
        perror("Error of calling listen");
        exit(1);
    }
    
    struct sockaddr_in clnt_addr;
    socklen_t addrlen = sizeof(clnt_addr);
    memset(&clnt_addr, '\0', sizeof(clnt_addr));
    
    int newsocket = -1;
    
    while (1)
    {
        printf("Ready to accept connections...\n");

        if ((newsocket = accept(sockfd, (struct sockaddr *)&clnt_addr, &addrlen)) == -1)
        {
            perror("Error of calling accept");
            exit(1);
        }

        printf("Client = %s \n", inet_ntoa(clnt_addr.sin_addr));

        struct server_state s;
        memset(&s, 0, sizeof(s));
        s.state = STATE_RECEIVE_COMMAND;

        short closed = 0;

        while (closed == 0)
        {
            switch (s.state)
            {
            case STATE_FINAL:
                close(newsocket);
                printf("Connection to client closed!\n");
                closed = 1;
                break;
            case STATE_SUCCESS:
                printf("STATE_SUCCESS\n");
                s.state = STATE_FINAL;
                break;
            case STATE_RECEIVE_COMMAND:
                printf("STATE_RECEIVE_COMMAND\n");
                memset(s.buffer, '\0', BUFFER_LENGTH);
                s.fd = newsocket;
                s.position_in_buffer = s.buffer;
                s.remain = COMMAN_LENGTH;
                s.desc_index = -1;
                get_command(&s);
                break;
            case STATE_RECEIVE_HOROSCOPE:
                printf("STATE_RECEIVE_HOROSCOPE\n");
                memset(s.buffer, '\0', BUFFER_LENGTH);
                s.fd = newsocket;
                s.position_in_buffer = s.buffer;
                s.remain = BUFFER_LENGTH;
                get_horoscope(&s);
                break;
            case STATE_SEND_HOROSCOPE:
                {
                    printf("STATE_SEND_HOROSCOPE\n");
                    memset(s.buffer, '\0', BUFFER_LENGTH);

                    if (s.desc_index > -1 && sizeof(descriptions[s.desc_index]) > 1)
                    {
                        memcpy(s.buffer, descriptions[s.desc_index], sizeof(descriptions[s.desc_index]) + 1);
                        s.state = STATE_SENDING;
                    }
                    else
                    {
                        s.state = STATE_SEND_SORRY;
                    }
                }
                break;
            case STATE_SEND_THANKS:
                printf("STATE_SEND_THANKS\n");
                memset(s.buffer, '\0', BUFFER_LENGTH);
                memcpy(s.buffer, "THANKS!", 8);
                s.state = STATE_SENDING;
                break;
            case STATE_SEND_DENINED:
                printf("STATE_SEND_DENINED\n");
                memset(s.buffer, '\0', BUFFER_LENGTH);
                memcpy(s.buffer, "DENIED!", 8);
                s.state = STATE_SENDING;
                break;
            case STATE_SEND_SORRY:
                printf("STATE_SEND_SORRY, index %d, horoscope len %d\n", s.desc_index, sizeof(descriptions[s.desc_index]));
                memset(s.buffer, '\0', BUFFER_LENGTH);
                memcpy(s.buffer, "SORRY!", 7);
                s.state = STATE_SENDING;
                break;
            case STATE_SENDING:
                printf("STATE_SENDING\n");
                s.position_in_buffer = s.buffer;
                s.remain = strlen(s.buffer) + 1;
                send_and_close(&s);
                break;
            case STATE_ERROR:
                printf("Error happened!\n");
                s.state = STATE_FINAL;
                break;
            default:
                //assert(0);
                printf("assert(0)\n");
                s.state = STATE_FINAL;
                closed = 1;
                break;
            }
        }
    }
    
    printf("Server stopped!");
    close(sockfd);
    exit(0);
}