#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <netdb.h>
#include <argp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <poll.h>

#define DEFAULT_BUFFER_SIZE 200 * 1024
#define DEFAULT_PORT 9999
#define DEFAULT_LOG "exampleauxd"
#define DEFAULT_DEST_HOST "127.0.0.1"
#define DEFAULT_DEST_PORT 9990

int create_socket(int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_result = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (bind_result == -1)
    {
        syslog(LOG_ERR, "Failed to bind the socket");
        exit(EXIT_FAILURE);
    }

    int listen_result = listen(server_socket, 5);
    if (listen_result == -1)
    {
        syslog(LOG_ERR, "Failed to listed the socket");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "server started");
    return server_socket;
}

int connect_sockets(char *destination_host, int destination_port)
{
    int client_socket;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
        syslog(LOG_ERR, "Client socket creation failed...");
    else
        syslog(LOG_INFO, "Client socket successfully created..");
    
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(destination_host);
    servaddr.sin_port = htons(destination_port);
 
    // connect the client socket to server socket
    if (connect(client_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0)
        syslog(LOG_ERR, "Connection with the server failed...");
    else
        syslog(LOG_INFO, "connected to the server..");

    return client_socket;
}

int send_data(char *destination_host, int destination_port, char *content, int len)
{
    int client_socket = connect_sockets(destination_host, destination_port);
 
    write(client_socket, content, len);
 
    // close the socket
    close(client_socket);
}

int main_loop(int server_socket, int buffer_size, char *destination_host, int destination_port)
{    
    struct pollfd poll_fds[1];
    poll_fds[0].fd = server_socket;
    poll_fds[0].events = POLLIN | POLLPRI;

    while (1)
    {
        int poll_result = poll(poll_fds, 1, -1);
        if (poll_result >= 0)
        {
            if (poll_fds[0].revents & POLLIN)
            {
                struct sockaddr_in cliaddr;
                int addrlen = sizeof(cliaddr);
                int client_socket = accept(server_socket, (struct sockaddr *)&cliaddr, &addrlen);
                syslog(LOG_INFO, "Successful connection from %s\n", inet_ntoa(cliaddr.sin_addr));

                poll_fds[0].fd = client_socket;
                poll_fds[0].events = POLLIN | POLLPRI;
                
                char buf[buffer_size];
                int len = read(poll_fds[0].fd, buf, buffer_size - 1);
                syslog(LOG_INFO, "Data size received %i", len);
                send_data(destination_host, destination_port, buf, len);
            }
        }
        // Sleep 0.1 seconds
        usleep(10000); 
    }    
}

int main(int argc, char *argv[])
{
    openlog(DEFAULT_LOG, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
    int listen_port = DEFAULT_PORT;
    int buffer_size = DEFAULT_BUFFER_SIZE;
    char *destination_host = DEFAULT_DEST_HOST;
    int destination_port = DEFAULT_DEST_PORT;

    size_t optind;
    for (optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
        case 'p': listen_port = atoi(argv[optind+1]); break;
        case 's': buffer_size = atoi(argv[optind+1]); break;
        case 'd': destination_port = atoi(argv[optind+1]); break;
        case 't': destination_host = argv[optind+1]; break;
        default:
            printf("Usage: %s [-p listen-port] [-s buffer-size] [-d destination-port] [-t destination-host]\n", argv[0]);
            exit(EXIT_FAILURE);
        }   
    }
    argv += optind;

    syslog(LOG_INFO, "Listening to port: %i\n", listen_port);
    syslog(LOG_INFO, "Sending received data to: %s:%i\n", destination_host, destination_port);

    int server_socket = create_socket(listen_port);
    int client_socket = main_loop(server_socket, buffer_size, destination_host, destination_port);

    syslog(LOG_INFO, "Server end\n");

    close(client_socket);
    close(server_socket);
    closelog();

    return 0;
}
