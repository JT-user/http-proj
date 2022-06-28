//
// Created by jonas on 26.06.2022.
#include "server.h"
#define _GNU_SOURCE
#include <stdint.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>

#define SERV_TIMEOUT (10) // 10ms time out
#define POLL_INIT (50) // initial size of epoll

/** running state */
bool running;

/** ptr to request handling function */
static int (*request_handler)(fd_t) = NULL;

static fd_t epoll_fd = FD_INVAL;

fd_t server_setup (server_opt_t serv_opts,int (*req_handler)(fd_t)) {
    int rv;
    uint16_t port = 31337;

    request_handler = req_handler;

    fd_t serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(serv_sock == ERROR)
    {
        perror("create-socket in server-setup");
        exit(EXIT_FAILURE);
    }

    char opt = true;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) );

    fcntl(serv_sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in serv_sock_addr = {0};
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = INADDR_ANY;
    serv_sock_addr.sin_port = htons(port);

    rv = bind(serv_sock, (const struct sockaddr *) &serv_sock_addr, sizeof(serv_sock_addr));
    if(rv == ERROR)
    {
        perror("bind-socket in server-setup");
        exit(EXIT_FAILURE);
    }


    rv = listen(serv_sock, MAX_QUEUED_REQ);
    if(rv == ERROR)
    {
        perror("listen-socket in server-setup");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event = {0};

    epoll_fd = epoll_create(POLL_INIT);

    event.events = (EPOLLIN);
    event.data.fd = serv_sock;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,serv_sock,&event);

    running = true;
    printf("server: started\n");
    return serv_sock;
}

void server_loop (server_opt_t serv_opts,fd_t serv_sock)
{
    if(epoll_fd == FD_INVAL)
        exit(EXIT_FAILURE);

    //#pragma omp parallel default(none) shared(serv_opts,serv_sock,epoll_fd,request_handler,running)
    {

        while(running)
        {

            struct epoll_event event = {0};

            fd_t epoll_rv = epoll_wait(epoll_fd,&event,1,SERV_TIMEOUT);
            if((epoll_rv == ERROR && (errno == EINTR)) || epoll_rv == 0)
                continue;
            else if (epoll_rv == ERROR)
            {
                perror("epoll_wait in server-loop");
                continue;
            }
            if( event.data.fd  == serv_sock)
            {
                struct sockaddr_in client_sock_addr = {0};
                socklen_t client_socklen = {0};

                fd_t  client_sock = accept4(serv_sock,
                                            (struct sockaddr *)&client_sock_addr,
                                            &client_socklen,
                                            SOCK_NONBLOCK);
                if(client_sock == ERROR)
                {
                    if((errno == EAGAIN)||(errno == EWOULDBLOCK))
                        continue;

                    perror("accept in server-loop");
                    continue;
                }
                event.events = (EPOLLIN | EPOLLEXCLUSIVE | EPOLLET);
                event.data.fd = client_sock;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_sock,&event);
                printf("accepted fd: %i\n",client_sock);
            }
            else
            {
                fd_t client_sock = event.data.fd;
                bool keep_alive = true;

                //TODO: check if socket is closed!

                //handle request
                int hrv = request_handler(client_sock);
                if (hrv != EAGAIN && hrv != EWOULDBLOCK && hrv != 0)
                    keep_alive = false;

                if(keep_alive) // if not keep alive -> close connection to client
                {
                    event.events = (EPOLLIN);
                    event.data.fd = client_sock;
                    epoll_ctl(epoll_fd,EPOLL_CTL_MOD,client_sock,&event);
                }
                else
                {
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_sock,NULL);
                    close(client_sock);
                }
            }
        }
    }
    printf("server: stopped\n");
    close(epoll_fd);
    epoll_fd = FD_INVAL;
    exit(EXIT_SUCCESS);
}

void server_stop ()
{
    running = false;
}
