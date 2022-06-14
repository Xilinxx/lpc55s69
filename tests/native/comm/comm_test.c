#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "comm_protocol.h"
#include "socket.h"
#include "logger.h"
#include "crc_linux.h"
#include "storage.h"
#include "storage_linux.h"

struct bootloader_ctxt_t _bctxt;
struct spi_ctxt_t _spictxt;

int _server_func(struct conn_t * conn) {
    if (!conn) {
        LOG_ERROR("Failed to start server");
        return -1;
    }

    struct storage_driver_t * sdriver = storage_new_linux_fd_driver(
        "test.txt");
    if (!sdriver) {
        LOG_ERROR("Failed to create storage drivers");
    }
    storage_init_storage(sdriver);

    struct comm_driver_t * drv = comm_protocol_find_driver("tcp");
    if (!drv) {
        LOG_ERROR("Driver not found");
        return -1;
    }
    COMM_SETPRIV(drv, conn);

    int error = comm_protocol_init();
    if (error < 0) {
        LOG_ERROR("Protocol init error");
        return -1;
    }

    error = comm_protocol_run(&_bctxt, drv, sdriver, sdriver, &_spictxt);
    if (error < 0) {
        LOG_ERROR("Protocol run error");
        return -1;
    }

    return 0;
}

int _client_func(struct conn_t * conn, char * filename) {
    if (!conn) {
        LOG_ERROR("Failed to start client");
        return -1;
    }

    struct comm_driver_t * drv = comm_protocol_find_driver("tcp");

    if (!drv) {
        LOG_ERROR("Driver not found");
        return -1;
    }
    COMM_SETPRIV(drv, conn);

    int error = comm_protocol_init();

    if (error < 0) {
        LOG_ERROR("Failed to init comm protocol");
        return -1;
    }

    int fp = open(filename, O_RDONLY);

    if (fp < 0) {
        LOG_ERROR("Failed to open file");
        return 0;
    }
    struct stat st;

    fstat(fp, &st);

    char * buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fp, 0);

    if (!buf) {
        LOG_ERROR("Failed to mmap file");
        return 0;
    }

    uint32_t crc32 = calculate_bin_crc(filename);

    error =
        comm_protocol_transfer_binary(drv, (uint8_t *)buf, st.st_size,
                                      0,
                                      crc32);

    return 0;
}

#define MAX_FILE_NAME 128

int main(int argc, char * argv[]) {
    int c;
    int port = -1;
    bool is_server = false;
    char ip[IP_ADDR_SIZE + 1] = { 0 };
    char filename[MAX_FILE_NAME + 1] = { 0 };

    LOG_INFO("Running comm test");

    while ((c = getopt(argc, argv, "sp:i:f:")) != -1) {
        switch (c) {
            case 's':
                is_server = true;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'i':
                strncpy(ip, optarg, IP_ADDR_SIZE);
                break;
            case 'f':
                strncpy(filename, optarg, MAX_FILE_NAME);
                break;
            default:
                LOG_ERROR("Invalid param: %c!", c);
                return -1;
        }
    }

    if (port < 0 || 0 == strlen(ip)) {
        LOG_ERROR("Invalid parameters, usage: comm_test -i <IP>" \
                  " -p <PORT> (-s for server)");
        return -1;
    }

    struct conn_t * conn = NULL;

    if (is_server) {
        LOG_INFO("listening on: %s:%d", ip, port);
        conn =
            socket_init_connection(ROLE_SERVER, IPPROTO_UDP, ip,
                                   port);
        _server_func(conn);
    } else {
        if (strlen(filename) == 0) {
            LOG_ERROR("Client needs filename");
            return -1;
        }

        LOG_INFO("Connecting to: %s:%d", ip, port);
        conn =
            socket_init_connection(ROLE_CLIENT, IPPROTO_UDP, ip,
                                   port);
        _client_func(conn, filename);
    }

    if (!conn) {
        LOG_ERROR("Failed to create connection");
        return -1;
    }

    socket_delete_connection(conn);

    return 0;
}
