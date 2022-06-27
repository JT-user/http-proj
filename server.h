//
// Created by jonas on 26.06.2022.
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdbool.h>

/** type for file descriptors */
typedef int fd_t;

#define FD_INVAL (-1)
#define ERROR (-1)

/** struct for server options */
typedef struct server_opt {
    bool dummy;
    //TODO: options to load from file in main
} server_opt_t;

#define MAX_QUEUED_REQ (256)

/**
 *
 * @return
 */
fd_t server_setup (server_opt_t serv_opts,void (*request_handler)(fd_t));

/**
 *
 * @return
 */
void server_loop (server_opt_t serv_opts, fd_t serv_sock);

void server_stop ();

#endif //HTTP_SERVER_H
