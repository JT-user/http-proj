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

#define SERV_TIMEOUT (50) // 10ms time out
#define POLL_INIT (256) // initial size of epoll

/** running state */
bool running;

/** ptr to request handling function */
static int (*request_handler)(fd_t) = NULL;

/** epoll handle */
static fd_t epoll_fd = FD_INVAL;

/** server options */
static  server_opt_t server_opts = {0};

fd_t server_setup (server_opt_t serv_opts, int (*req_handler)(fd_t)) {
    int rv;
    uint16_t port = 31337;

    server_opts = serv_opts;
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

void server_loop (fd_t serv_sock)
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
                    break;
                }
                //enqueue
                event.events = (EPOLLIN | EPOLLONESHOT | EPOLLRDHUP);
                event.data.fd = client_sock;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_sock,&event);
                printf("accepted fd: %i\n",client_sock);
            }
            else
            {
                fd_t client_sock = event.data.fd;
                uint32_t client_event = event.events;

                // if socket got closed!
                if(client_event == EPOLLRDHUP)
                {
                    printf("client socket %i disconnected\n",client_sock);
                    goto remove_client_socket;
                }

                bool keep_alive = true;

                //handle request
                int hrv = request_handler(client_sock);

                if (hrv != EAGAIN && hrv != EWOULDBLOCK && hrv != 0)
                    goto remove_client_socket;

                if(keep_alive) // if not keep alive -> close connection to client
                {
                    // requeue_client_socket
                    event.events = (EPOLLIN | EPOLLONESHOT | EPOLLRDHUP);
                    event.data.fd = client_sock;
                    epoll_ctl(epoll_fd,EPOLL_CTL_MOD,client_sock,&event);
                    continue;
                }

                remove_client_socket:
                {
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,client_sock,NULL);
                    close(client_sock);
                }
            }
        }
        //#pragma omp barrier
    }
    printf("server: stopped\n");
    close(epoll_fd);
    epoll_fd = FD_INVAL;
}

void server_stop ()
{
    running = false;
}