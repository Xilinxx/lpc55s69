/**
 * @file logger_conf.c
 * @brief  Configuration for the logger
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-11
 */

#include <stddef.h>

#include "logger.h"

#if defined(CFG_LOGGER_SIMPLE_LOGGER) && !defined(CFG_LOGGER_ADV_LOGGER)
extern struct logger_driver_t stdio_logger;
#endif /* CFG_LOGGER_SIMPLE_LOGGER && !CFG_LOGGER_ADV_LOGGER */

extern struct logger_driver_t uart_logger;
extern struct logger_driver_t app_mem_logger;

struct logger_driver_t *adrivers[] = {
#if defined(CFG_LOGGER_SIMPLE_LOGGER) && !defined(CFG_LOGGER_ADV_LOGGER)
	&stdio_logger,
#endif /* CFG_LOGGER_SIMPLE_LOGGER && !CFG_LOGGER_ADV_LOGGER */
	&uart_logger,
	NULL,
};
