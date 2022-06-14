/**
 * @file storage_linux_file.c
 * @brief Linux storage driver (File based)
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-10-14
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "storage.h"
#include "logger.h"

#define MAX_FILE_NAME 128

struct fd_ctxt_t {
    FILE * fd;
    char filestr[MAX_FILE_NAME];
    int written;
};

static int _init_file_storage(struct storage_driver_t * sdriver) {
    LOG_INFO("Running file backed storage driver");
    struct fd_ctxt_t * ctxt = (struct fd_ctxt_t *)STORAGE_GETPRIV(sdriver);
    if (!ctxt) {
        LOG_ERROR("Context cannot be NULL");
        return -1;
    }

    ctxt->fd = fopen(ctxt->filestr, "wb+");
    if (!ctxt->fd) {
        LOG_ERROR("Failed to open file");
        return -1;
    }

    return 0;
}

static int _read_file_storage(struct storage_driver_t * sdriver, uint8_t * buffer,
                              size_t len) {
    LOG_INFO("Read from file backed storage driver");
    return 0;
}

static int _write_file_storage(struct storage_driver_t * sdriver,
                               uint8_t * buffer, size_t len) {
    LOG_INFO("Write to file backed storage driver");
    struct fd_ctxt_t * ctxt = (struct fd_ctxt_t *)STORAGE_GETPRIV(sdriver);
    if (!ctxt) {
        LOG_ERROR("Context cannot be NULL");
        return -1;
    }

    int written = fwrite(buffer, sizeof(uint8_t), len, ctxt->fd);
    LOG_DEBUG("Written %d", written);
    ctxt->written += written;
}

static int _flush_file_storage(struct storage_driver_t * sdriver) {
    LOG_DEBUG("Flushing storage driver");
    struct fd_ctxt_t * ctxt = (struct fd_ctxt_t *)STORAGE_GETPRIV(sdriver);
    if (!ctxt) {
        LOG_ERROR("Context cannot be NULL");
        return -1;
    }
    LOG_DEBUG("Written %d", ctxt->written);
    ctxt->written = 0;
    return fflush(ctxt->fd);
}

static int _erase_file_storage(struct storage_driver_t * sdriver) {
    LOG_INFO("Erasing file backend");
    return 0;
}

static int _close_file_storage(struct storage_driver_t * sdriver) {
    LOG_INFO("Closing file backed storage driver");

    struct fd_ctxt_t * ctxt = (struct fd_ctxt_t *)STORAGE_GETPRIV(sdriver);
    if (!ctxt) {
        LOG_ERROR("Context cannot be NULL");
        return -1;
    }
    fclose(ctxt->fd);
    return 0;
}

static uint32_t _crc_file_storage(struct storage_driver_t * sdriver) {
    return 0;
}

static const struct storage_ops_t fdops = {
    .init  = _init_file_storage,
    .read  = _read_file_storage,
    .write = _write_file_storage,
    .erase = _erase_file_storage,
    .flush = _flush_file_storage,
    .crc   = _crc_file_storage,
    .close = _close_file_storage,
};

struct storage_driver_t * storage_new_linux_fd_driver(char * filename) {
    LOG_INFO("Creating new storage driver");

    struct storage_driver_t * sdriver = NULL;
    sdriver = malloc(sizeof(struct storage_driver_t));
    if (!sdriver) {
        LOG_ERROR("Failed to allocate driver structure");
        goto error;
    }
    memset(sdriver, 0, sizeof(struct storage_driver_t));

    struct fd_ctxt_t * ctxt = malloc(sizeof(struct fd_ctxt_t));
    if (!ctxt) {
        LOG_ERROR("Failed to create file ctxt");
        goto error;
    }
    memset(ctxt, 0, sizeof(struct fd_ctxt_t));

    strncpy(ctxt->filestr, filename, MAX_FILE_NAME);

    sdriver->ops = &fdops;
    STORAGE_SETPRIV(sdriver, ctxt);

    LOG_INFO("Openening %s as backend", ctxt->filestr);
    return sdriver;

 error:
    if (sdriver) {
        if (sdriver->privdata) {
            free(sdriver->privdata);
        }
        free(sdriver);
    }
    return NULL;
}
