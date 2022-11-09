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
#define DEFAULT_PORT 9990
#define DEFAULT_PATH "/var/log/exampled.dat"
#define DEFAULT_LOG "exampled"

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

int write_data(char *data_path, char *content, int len)
{
    FILE *fptr;
    fptr = fopen(data_path, "a");

    if(fptr == NULL)
    {
      syslog(LOG_ERR, "Failed to open the data file");   
      exit(EXIT_FAILURE);             
    }

    for (int i = 0; i < len; ++i) {
        fprintf(fptr, "%c", content[i]);
    }
    fclose(fptr);
}

int main_loop(int server_socket, char *data_path, int buffer_size)
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
                write_data(data_path, buf, len);
            }
        }
        // Sleep 0.1 seconds
        usleep(10000); 
    }    
}

int main(int argc, char *argv[])
{
    openlog(DEFAULT_LOG, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
    int port = DEFAULT_PORT;
    int buffer_size = DEFAULT_BUFFER_SIZE;
    char *data_path = DEFAULT_PATH;

    size_t optind;
    for (optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
        case 'l': data_path = argv[optind+1]; break;
        case 'p': port = atoi(argv[optind+1]); break;
        case 's': buffer_size = atoi(argv[optind+1]); break;
        default:
            printf("Usage: %s [-l file] [-p port] [-s buffer-size]\n", argv[0]);
            exit(EXIT_FAILURE);
        }   
    }
    argv += optind;

    syslog(LOG_INFO, "Listening to port: %i\n", port);
    syslog(LOG_INFO, "Logging to file: %s\n", data_path);

    int server_socket = create_socket(port);
    int client_socket = main_loop(server_socket, data_path, buffer_size);

    syslog(LOG_INFO, "Server end\n");

    close(client_socket);
    close(server_socket);
    closelog();

    return 0;
}
