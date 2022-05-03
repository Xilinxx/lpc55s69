/**
 * @file logger-stdio.c
 * @brief  Simple stdio driver for logger
 *
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v3.0
 * @date 2020-08-10
 */

#include <stdio.h>
#include <string.h>

#include "logger.h"

char simple_logger_buffer[MAX_STR_LEN + 1] = { 0 };

#define MAX_HDR_LEN 128

static void _print_header(struct line_info_t *linfo)
{
	memset(simple_logger_buffer, 0, MAX_STR_LEN + 1);
	snprintf(simple_logger_buffer, MAX_HDR_LEN,
		 "[%s%5s%s] (%20s)(%30s @%3d) : ",
		 _log_levels[logger_mask2id(linfo->lvl)].color,
		 _log_levels[logger_mask2id(linfo->lvl)].name,
		 RESET, linfo->file, linfo->fn, linfo->ln);
	fprintf(stdout, "%s", simple_logger_buffer);
}

static int _printf_wrapper(void *priv, struct line_info_t *linfo, char *fmt,
			   va_list *v)
{
	(void)priv;

	if (linfo->lvl != LOG_LVL_RAW) {
		_print_header(linfo);
	}

	memset(simple_logger_buffer, 0, MAX_STR_LEN + 1);
	vsnprintf(simple_logger_buffer, MAX_STR_LEN, fmt, *v);
	fprintf(stdout, "%s\n", simple_logger_buffer);

	return 0;
}

static const struct logger_ops_t stdio_ops = {
	.init	= NULL,
	.write	= _printf_wrapper,
	.read	= NULL,
	.flush	= NULL,
	.close	= NULL,
};

struct logger_driver_t stdio_logger = {
	.enabled	= true,
	.name		= "stdio",
	.ops		= &stdio_ops,
	.priv_data	= NULL,
};
