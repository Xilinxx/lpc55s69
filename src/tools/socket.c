/**
 * @file socket.c
 * @brief  Simple socket abstraction
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2015-12-15
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>

#include "socket.h"
#include "logger.h"

#define BUF_SIZE 2048

struct conn_t * socket_init_connection(role_t role,
                                       int protocol,
                                       char * ip,
                                       int port) {
    struct conn_t * con;

    con = malloc(sizeof(struct conn_t));
    if (!con) {
        LOG_ERROR("Failed to allocate memory for connection struct");
        return NULL;
    }
    memset(con, 0, sizeof(struct conn_t));

    con->role = role;
    con->protocol = protocol;
    if (ip) {
        memset(con->ip, 0, IP_ADDR_SIZE + 1);
        strncpy(con->ip, ip, IP_ADDR_SIZE);
    } else {
        if (role == ROLE_CLIENT) {
            LOG_ERROR("Client should have IP address");
            free(con);
            return NULL;
        }
    }
    con->port = port;
    return con;
}

int socket_create_socket(struct conn_t * con,
                         int options) {
    struct hostent * server;

    if (con->protocol == IPPROTO_UDP) {
        LOG_INFO("Creating UDP Socket");
        con->sock = socket(AF_INET, SOCK_DGRAM, 0);
    } else if (con->protocol == IPPROTO_TCP) {
        LOG_INFO("Creating TCP Socket");
        con->sock = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (con->sock < 0) {
        LOG_ERROR("Can't create socket");
        return -1;
    }

    if ((options & SOCK_BROADCAST) != 0) {
        socket_setup_broadcast(con->sock);
    }

    if ((options & SOCK_REUSEADDR) != 0) {
        socket_setup_reuseaddr(con->sock);
    }
    memset(&con->c, 0, sizeof(struct sockaddr_in));
    memset(&con->s, 0, sizeof(struct sockaddr_in));
    con->s.sin_family = AF_INET;
    con->s.sin_port = htons(con->port);

    if (con->role == ROLE_CLIENT) {
        server = gethostbyname(con->ip);
        if (!server) {
            LOG_ERROR("Could not resolve hostname");
            return -1;
        }
        bcopy(server->h_addr, &con->s.sin_addr.s_addr,
              server->h_length);
        if (con->protocol == IPPROTO_TCP) {
            socklen_t len = sizeof(con->s);
            LOG_ERROR("Trying to connect");
            int status = connect(con->sock,
                                 (struct sockaddr *)&con->s, len);
            if (status < 0) {
                LOG_ERROR("Failed to connect to server");
                return -1;
            }
        }
    } else {
        con->s.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(con->sock, (struct sockaddr *)&con->s,
                 sizeof(con->s)) == -1) {
            LOG_ERROR("Failed to bind port");
            return -1;
        }
        LOG_INFO("Socket binded to port");
        if (con->protocol == IPPROTO_TCP) {
            if (listen(con->sock, 10) < 0) {
                LOG_ERROR("Failed to start listen");
                return -1;
            }
        }
    }
    LOG_OK("Socket created");
    return 1;
}

int socket_setup_broadcast(int sock) {
    int broadcast = 1;
    int ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                         sizeof(broadcast));

    if (ret < 0) {
        LOG_ERROR("Failed to setup broadcast");
        return -1;
    }
    return 1;
}

int socket_setup_reuseaddr(int sock) {
    int enable = 1;
    int ret =
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable,
                   sizeof(enable));

    if (ret < 0) {
        LOG_ERROR("Failed to setup reuseaddr");
        return -1;
    }
    return 1;
}

void socket_delete_connection(struct conn_t * conn) {
    if (!conn) {
        LOG_WARN("Connection struct is NULL");
        return;
    }
    close(conn->sock);
    free(conn);
}
