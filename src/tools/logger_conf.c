/**
 * @file logger_conf.c
 * @brief  Configuration for the logger
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-11
 */

#include <stddef.h>

#include "logger.h"

extern struct logger_driver_t stdio_logger;

struct logger_driver_t * adrivers[] = {
    &stdio_logger,
    NULL,
};
