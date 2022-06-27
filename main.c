//
// Created by Jonas Thalmann on 20.06.2022.
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ip.h>

#include "server.h"

void http_request_entry(int client_sock_fd);

static void handle_signal(int signum)
{
    if (signum == SIGINT) {
        printf("signal INT received!\n");
        server_stop();
        return;
    }
    if (signum == SIGTERM ) {
        printf("signal TERM received!\n");
        server_stop();
        return;
    }

    printf("signal-handler: unknown signal\n");
    exit(EXIT_FAILURE);
}

int main()
{
    if(
        signal(SIGINT, handle_signal) == SIG_ERR ||
        signal(SIGTERM, handle_signal) == SIG_ERR )
    {
        perror("register signal handler in main: ");
    }
    server_opt_t serv_opts = {.dummy = false};

    fd_t serv_sock = server_setup(serv_opts,http_request_entry);

    server_loop(serv_opts,serv_sock);

    printf("server-loop: should not return\n");
    exit(EXIT_FAILURE);
}

void log_message(char * msg, size_t msg_len)
{
    write(STDOUT_FILENO, msg, msg_len - 1);
}

void http_request_entry(int client_sock_fd)
{
    char request[1024];
    char response[] = "HTTP/1.0 200 OK\n"
                    "Server: HTSuSExP/0.0.1 (Unix)\n"
                    "Content-Type: text/html\n"
                    "Content-Length: 32\n"
                    "\n"
                    "<html>THIS IS A TEST PAGE</html>\n\n";

    recv(client_sock_fd,request,1024,0);

    send(client_sock_fd,response, sizeof(response),0);

    printf("msg sent to fd: %i\n", client_sock_fd);
}
