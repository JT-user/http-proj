//
// Created by Jonas Thalmann on 20.06.2022.
#include <unistd.h>
#include <signal.h>
#include <netinet/ip.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

int http_request_entry(int client_sock_fd);

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

int http_request_entry(int client_sock_fd)
{
    #include <omp.h>
    char request[1024];
    char response[] = "HTTP/1.0 200 OK\r\n"
                    "Server: HTSuSExP/0.0.1 (Unix)\r\n"
                    "Connection: keep-alive\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: 32\r\n"
                    "\r\n"
                    "<html>THIS IS A TEST PAGE</html>";

    ssize_t rrv = recvfrom(client_sock_fd,request,1024,MSG_DONTWAIT,NULL,NULL);
    if(rrv == ERROR)
    {
        perror("recv");
        return errno;
    }
    if(rrv==0)
    {
        return -1;
        printf("client socket %i disconnected\n",client_sock_fd);
    }


    ssize_t srv = send(client_sock_fd,response, sizeof(response)-1,MSG_NOSIGNAL);
    if(srv == ERROR)
    {
        perror("send");
        return errno;
    }

    char * save = NULL;
    strtok_r(request,"\r\n",&save);
    printf("req: %s recv: %zi, send: %zi, thread %i\n",request, rrv,srv, omp_get_thread_num());
    return 0;
}
