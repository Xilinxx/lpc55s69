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
	uint32_t *	mem_base;       //!< Base address of Shared memory
	uint32_t *	mem_start;      //!< Start of logging region (after config struct)
	uint32_t *	mem_end;        //!< End of shared memory
	uint32_t *	curr_offset;    //!< Current log pointer
	uint32_t	count;          //!< Number of messages logged
};


/**
 * @brief  Default logger context struct
 */
static struct mem_ctxt_t _default_ctxt = {
	.mem_base	= (uint32_t *)&__ssram_start__,
	.mem_start	= (uint32_t *)&__ssram_start__,
	.mem_end	= (uint32_t *)&__ssram_end__,
	.curr_offset	= NULL,
	.count		= 0,
};

/**
 * @brief Wipe the configuration region
 *
 * @param start Start of the configuration region
 * @param end End of the configuration region
 */
static void _wipe_config_region(uint32_t *start, uint32_t *end)
{
	uint32_t *curr = start;

	while (curr != end) {
		*curr++ = 0xFFFFFFFF; //!< wipe data
	}
}

/**
 * @brief Wipe the logging region
 *
 * @param start Start of the logging region
 * @param end End of the logging region
 */
static void _wipe_old_logging(uint32_t *start, uint32_t *end)
{
	uint32_t *curr = start;

	while (curr != end) {
		*curr++ = 0x0; //!< wipe data
	}
}

/**
 * @brief  Initialize the memory logger if in bootloader mode
 *
 * @param drv Driver which will be initialized
 *
 * @returns  -1 if failed otherwise 0
 */
static int _init_bootloader_mem(void *drv)
{
	struct logger_driver_t *driver = (struct logger_driver_t *)drv;

	if (!driver) {
		return -1;
	}

	struct mem_ctxt_t *ctxt = (struct mem_ctxt_t *)&__ssram_start__;

	_wipe_config_region(ctxt->mem_base, ctxt->mem_start);

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
static int _init_application_mem(void *drv)
{
	struct logger_driver_t *driver = (struct logger_driver_t *)drv;

	if (!driver) {
		return -1;
	}

	struct mem_ctxt_t *ctxt = (struct mem_ctxt_t *)&__ssram_start__;

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
static int _write_mem(void *drv, struct line_info_t *linfo, char *fmt,
		      va_list *v)
{
	(void)linfo;

	struct logger_driver_t *driver = (struct logger_driver_t *)drv;

	if (!driver) {
		return -1;
	}
	struct mem_ctxt_t *ctxt = (struct mem_ctxt_t *)driver->priv_data;

	*ctxt->curr_offset = __builtin_bswap32(ctxt->count);

	char *mem_reg = (char *)ctxt->curr_offset;

	vsnprintf(&mem_reg[4], MAX_LOG_LEN, fmt, *v);

	if (ctxt->curr_offset == ctxt->mem_end) {
		ctxt->curr_offset = ctxt->mem_start;
	} else {
		ctxt->curr_offset += (MAX_LOG_LEN / sizeof(uint32_t));
	}
	ctxt->count++;

	return 0;
}

static const struct logger_ops_t mem_ops = {
	.init	= _init_bootloader_mem,
	.write	= _write_mem,
	.read	= NULL,
	.flush	= NULL,
	.close	= NULL,
};

struct logger_driver_t mem_logger = {
	.enabled	= true,
	.name		= "memory",
	.ops		= &mem_ops,
	.priv_data	= NULL,
};
