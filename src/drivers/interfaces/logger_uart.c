/**
 * @file logger_uart.c
 * @brief  UART driver for logger
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-08-12
 */

#include <stdio.h>
#include <string.h>

#include "peripherals.h"
#include "fsl_usart.h"
#include "logger.h"

char uart_logger_buffer[MAX_STR_LEN + 1] = { 0 };

#define MAX_HDR_LEN 128

struct uart_logger_ctxt_t {
	USART_Type *base;
};

static int _init_uart(void *drv)
{
	struct logger_driver_t *driver = (struct logger_driver_t *)drv;

	if (!driver) {
		return -1;
	}

	return 0;
}

static void _print_header(struct uart_logger_ctxt_t *	ctxt,
			  struct line_info_t *		linfo)
{
	memset(uart_logger_buffer, 0, MAX_STR_LEN + 1);
	snprintf(uart_logger_buffer, MAX_HDR_LEN,
		 "[%s%5s%s] (%20s)(%30s @%3d) : ",
		 _log_levels[logger_mask2id(linfo->lvl)].color,
		 _log_levels[logger_mask2id(linfo->lvl)].name,
		 RESET, linfo->file, linfo->fn, linfo->ln);
	USART_WriteBlocking(ctxt->base, (const uint8_t *)uart_logger_buffer,
			    strlen(uart_logger_buffer));
}

static int _write_uart(void *drv, struct line_info_t *linfo, char *fmt,
		       va_list *v)
{
	struct logger_driver_t *driver = (struct logger_driver_t *)drv;

	if (!driver) {
		return -1;
	}
	struct uart_logger_ctxt_t *ctxt =
		(struct uart_logger_ctxt_t *)driver->priv_data;

	if (linfo->lvl != LOG_LVL_RAW) {
		_print_header(ctxt, linfo);
	}

	memset(uart_logger_buffer, 0, MAX_STR_LEN + 1);
	vsnprintf(uart_logger_buffer, MAX_STR_LEN, fmt, *v);

	USART_WriteBlocking(ctxt->base, (const uint8_t *)uart_logger_buffer,
			    strlen(uart_logger_buffer));
	USART_WriteBlocking(ctxt->base, (const uint8_t *)"\r\n", 2);
	return 0;
}

static const struct logger_ops_t uart_ops = {
	.init	= _init_uart,
	.write	= _write_uart,
	.read	= NULL,
	.flush	= NULL,
	.close	= NULL,
};

static struct uart_logger_ctxt_t _ctxt = {
	.base	= UART_LOGGER,
};

struct logger_driver_t uart_logger = {
	.enabled	= true,
	.name		= "uart_logger",
	.ops		= &uart_ops,
	.priv_data	= &_ctxt,
};
