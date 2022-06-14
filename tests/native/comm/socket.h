/**
 * @file socket.h
 * @brief  Simple socket abstraction
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2015-12-15
 */

#ifndef _GEN_SOCKET_H_
#define _GEN_SOCKET_H_

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IP_ADDR_SIZE   16
#define SOCK_BROADCAST 0x01
#define SOCK_MULTICAST 0x02
#define SOCK_REUSEADDR 0x04

/**
 * Connection role enum
 */
typedef enum {
    ROLE_SERVER,
    ROLE_CLIENT,
} role_t;

/**
 * Connection structure
 */
struct conn_t {
    int sock;                                           // Socket
    int protocol;                                       // IPPROTO_UDP or IPPROTO_TCP
    role_t role;                                        // SERVER or CLIENT
    char ip[IP_ADDR_SIZE];                              // Connection IP, NULL if role == SERVER
    int port;                                           // Connection or listen port
    struct sockaddr_in s;                               // Sockaddr for this connection
    struct sockaddr_in c;
};

/**
 * Initialise a connection
 * @param role Server or client
 * @param protocol TCP or udp
 * @param ip The connection ip, NULL if role == server
 * @param port The connection port
 * @returns an initialised connection
 */
struct conn_t * socket_init_connection(role_t role, int protocol, char * ip, int
                                       port);

/**
 * Create socket from connection thread
 * @param con The connection struct that we will be using
 * @param options Added options for the socket
 * @return Returns an error_t value
 */
int socket_create_socket(struct conn_t * con, int options);

/**
 * Setup broadcast for a socket
 * @param socket The socket that will be set to broadcast
 * @return Returns an error_t
 */
int socket_setup_broadcast(int socket);

/**
 * Setup reuseaddr for a socket
 * @param socket The socket that will be set to broadcast
 * @return Returns an error_t
 */
int socket_setup_reuseaddr(int socket);

/**
 * Delete a socket connection
 */
void socket_delete_connection(struct conn_t * conn);

#endif /* _GEN_SOCKET_H_ */
