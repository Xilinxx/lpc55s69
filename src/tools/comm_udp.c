#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#include "socket.h"
#include "comm_driver.h"
#include "logger.h"

static int _comm_udp_init(void * drv) {
    struct comm_driver_t * driver = (struct comm_driver_t *)drv;

    if (!drv) {
        LOG_ERROR("No driver given");
        return -1;
    }

    struct conn_t * conn = (struct conn_t *)driver->priv_data;

    if (!conn) {
        LOG_ERROR("Invalid connection");
        return -1;
    }

    int err = socket_create_socket(conn, SOCK_REUSEADDR);

    if (err < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }


    return 0;
}

static int _comm_udp_write(void * drv,
                           uint8_t * buffer,
                           size_t len) {
    struct comm_driver_t * driver = (struct comm_driver_t *)drv;

    if (!drv) {
        LOG_ERROR("No driver given");
        return -1;
    }

    struct conn_t * conn = (struct conn_t *)driver->priv_data;

    if (!conn) {
        LOG_ERROR("Invalid connection");
        return -1;
    }

    struct sockaddr * dest = conn->role == ROLE_SERVER ? (struct
                                                          sockaddr *)&conn->c
        :
        (struct sockaddr *)&conn->s;

    int written = sendto(conn->sock, buffer, len, 0,
                         (struct sockaddr *)dest, sizeof(*dest));

    if (written < 0) {
        LOG_ERROR("Failed to send packet");
    }
    if (written != len) {
        LOG_ERROR("Did not write requested length");
    }
    return written;
}

static int _comm_udp_read(void * drv,
                          uint8_t * buffer,
                          size_t * len) {
    struct comm_driver_t * driver = (struct comm_driver_t *)drv;

    if (!drv) {
        LOG_ERROR("No driver given");
        return -1;
    }

    struct conn_t * conn = (struct conn_t *)driver->priv_data;

    if (!conn) {
        LOG_ERROR("Invalid connection");
        return -1;
    }

    int read = 0;
    struct sockaddr * src = conn->role == ROLE_SERVER ? (struct
                                                         sockaddr *)&conn->c
        :
        (struct sockaddr *)&conn->s;
    socklen_t socklen = sizeof(*src);

    if (conn->role == ROLE_SERVER) {
        read = recvfrom(conn->sock, (void *)buffer, *len, 0,
                        (struct sockaddr *)src, &socklen);
    } else {
        read = recvfrom(conn->sock, (void *)buffer, *len, 0,
                        (struct sockaddr *)src, &socklen);
    }

    if (read < 0) {
        LOG_ERROR("Failed to read packet");
    }
    if (read != *len) {
        LOG_DEBUG("Did not read requested length (%d - %d)",
                  read, *len);
    }
    *len = read;

    return read;
}

static void _comm_udp_close(void * drv) {
    return;
}

static const struct comm_ops_t _udp_ops = {
    .init  = _comm_udp_init,
    .write = _comm_udp_write,
    .read  = _comm_udp_read,
    .close = _comm_udp_close,
};

struct comm_driver_t udp_comm = {
    .enabled   = true,
    .name      = "udp",
    .ops       = &_udp_ops,
    .priv_data = NULL,
};
