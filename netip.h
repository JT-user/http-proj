#ifndef HTTP_NETIP_H
#define HTTP_NETIP_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
// Created by Jonas Thalmann on 20.06.2022.

#define MAX_QUEUED_REQ (256) //pow(2,7)
#define ERROR (-1)
#define LOG_MSG(msg) log_message(msg,sizeof(msg))

typedef struct ip_sock_opt {
    char test;
    //TODO: options to load from file in main
} ip_sock_opt_t;

int server_accept();

int server_();

static bool running = true;

void server(ip_sock_opt_t ip_sock_opts);

extern void http_request_entry(int fd, struct sockaddr_in in);

extern void handle_error(int error);

extern void log_message(char * msg, int msg_len);

void server(ip_sock_opt_t ip_sock_opts)
{
    if(ip_sock_opts.test != 'A')
        _exit(1);

    running = true;
    uint16_t port = 31337;

    int rv;
    //TODO: change test implementation

    int serv_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(serv_sock_fd == ERROR)
        handle_error(errno);

    char opt = true;
    setsockopt(serv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) );

    fcntl(serv_sock_fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in serv_sock_addr = {0};
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = INADDR_ANY;
    serv_sock_addr.sin_port = htons(port);

    rv = bind(serv_sock_fd, (const struct sockaddr *) &serv_sock_addr, sizeof(serv_sock_addr));
    if(rv == ERROR)
        handle_error(errno);

    rv = listen(serv_sock_fd, MAX_QUEUED_REQ);
    if(rv == ERROR)
        handle_error(errno);

    LOG_MSG("server started\n");

    //TODO: https://man7.org/linux/man-pages/man7/epoll.7.html
    do
    {
        struct sockaddr_in client_sock_addr = {0};
        socklen_t client_socklen = {0};

        if(!running) //TODO: test_running();
            break;

        //synchronize
        int client_sock_fd = accept(serv_sock_fd,(struct sockaddr *)&client_sock_addr,&client_socklen);


        if(client_sock_fd == ERROR)
            handle_error(errno);

        http_request_entry(client_sock_fd,client_sock_addr);
    }
    while (true);

    close(serv_sock_fd);
    _exit(0);

}

#endif //HTTP_NETIP_H
