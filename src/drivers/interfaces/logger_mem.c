/**
 * @file logger_mem.c
 * @brief  Memory logger
 *
 * This will store logging in memory. Imporant for logging when nor serial or uart is available
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-19
 */

#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "memory_map.h"

#define MAX_LOG_LEN 128

struct mem_ctxt_t {
    uint32_t * mem_base;                // !< Base address of Shared memory
    uint32_t * mem_start;               // !< Start of logging region (after config struct)
    uint32_t * mem_end;                 // !< End of shared memory
    uint32_t * curr_offset;             // !< Current log pointer
    uint32_t count;                     // !< Number of messages logged
};


/**
 * @brief  Default logger context struct
 */
static struct mem_ctxt_t _default_ctxt = {
    .mem_base    = (uint32_t *)&__ssram_log_start__,
    .mem_start   = (uint32_t *)&__ssram_log_start__,
    .mem_end     = (uint32_t *)&__ssram_log_end__,
    .curr_offset = NULL,
    .count       = 0,
};

/**
 * @brief Wipe the configuration region
 *
 * @param start Start of the configuration region
 * @param end End of the configuration region
 */
static void _wipe_config_region(uint32_t * start, uint32_t * end) {
    uint32_t * curr = start;

    while (curr != end)
        *curr++ = 0xFFFFFFFFU;         // !< wipe data
}

/**
 * @brief Wipe the logging region
 *
 * @param start Start of the logging region
 * @param end End of the logging region
 */
static void _wipe_old_logging(uint32_t * start, uint32_t * end) {
    uint32_t * curr = start;

    while (curr != end)
        *curr++ = 0x0U;         // !< wipe data
}

/**
 * @brief  Initialize the memory logger if in bootloader mode
 *
 * @param drv Driver which will be initialized
 *
 * @returns  -1 if failed otherwise 0
 */
static int _init_bootloader_mem(void * drv) {
    struct logger_driver_t * driver = (struct logger_driver_t *)drv;

    if (!driver) {
        return -1;
    }

    _wipe_config_region(&__ssram_log_start__,
                        (&__ssram_log_start__ +
                         (MAX_LOG_LEN / sizeof(uint32_t))));

    struct mem_ctxt_t * ctxt = (struct mem_ctxt_t *)&__ssram_log_start__;

    *ctxt = _default_ctxt;

    driver->priv_data = ctxt;

    ctxt->mem_start = ctxt->mem_base + (MAX_LOG_LEN / sizeof(uint32_t));
    ctxt->curr_offset = ctxt->mem_start;

    _wipe_old_logging(ctxt->mem_start, ctxt->mem_end);

    return 0;
}

/**
 * @brief  Initialize the memory logger if in application mode
 *
 * @param drv Driver which will be initialized
 *
 * @returns  -1 if failed otherwise 0
 */
static int _init_application_mem(void * drv) {
    struct logger_driver_t * driver = (struct logger_driver_t *)drv;

    if (!driver) {
        return -1;
    }

    struct mem_ctxt_t * ctxt = (struct mem_ctxt_t *)&__ssram_log_start__;

    driver->priv_data = ctxt;

    return 0;
}

/**
 * @brief Write to memory
 *
 * @param drv Driver which will be written
 *
 * @returns  -1 if failed otherwise 0
 */
static int _write_mem(void * drv, struct line_info_t * linfo, char * fmt,
                      va_list * v) {
    (void)linfo;

    struct logger_driver_t * driver = (struct logger_driver_t *)drv;

    if (!driver) {
        return -1;
    }
    struct mem_ctxt_t * ctxt = (struct mem_ctxt_t *)driver->priv_data;

    /* Note: __builtin_bswap32 is a gcc extension. If another compiler is used
     * this will cause issues */
    *ctxt->curr_offset = __builtin_bswap32(ctxt->count);

    char * mem_reg = (char *)ctxt->curr_offset;

    /* Offset with 4 Byte. first 4 bytes are a 32bit counter */
    vsnprintf(&mem_reg[sizeof(uint32_t)], MAX_LOG_LEN, fmt, *v);

    if (ctxt->curr_offset ==
        (ctxt->mem_end - (MAX_LOG_LEN / sizeof(uint32_t)))) {
        ctxt->curr_offset = ctxt->mem_start;
    } else {
        ctxt->curr_offset += (MAX_LOG_LEN / sizeof(uint32_t));
    }
    ctxt->count++;

    return 0;
}

static const struct logger_ops_t app_mem_ops = {
    .init  = _init_application_mem,
    .write = _write_mem,
    .read  = NULL,
    .flush = NULL,
    .close = NULL,
};

struct logger_driver_t app_mem_logger = {
    .enabled   = true,
    .name      = "memory",
    .ops       = &app_mem_ops,
    .priv_data = NULL,
};

static const struct logger_ops_t btl_mem_ops = {
    .init  = _init_bootloader_mem,
    .write = _write_mem,
    .read  = NULL,
    .flush = NULL,
    .close = NULL,
};

struct logger_driver_t btl_mem_logger = {
    .enabled   = true,
    .name      = "memory",
    .ops       = &btl_mem_ops,
    .priv_data = NULL,
};
