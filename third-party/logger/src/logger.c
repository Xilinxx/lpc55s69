/**
 * @file logger.c
 * @brief  Logger V3
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v3.0
 * @date 2020-08-10
 */

#include <string.h>

#include "logger.h"

#if !defined(CFG_LOGGER_EXTERNAL_DRIVER_CONF)
#if defined(CFG_LOGGER_SIMPLE_LOGGER) && !defined(CFG_LOGGER_ADV_LOGGER)
extern struct logger_driver_t stdio_logger;
#endif /* CFG_LOGGER_SIMPLE_LOGGER && !CFG_LOGGER_ADV_LOGGER */

static struct logger_driver_t *adrivers[] = {
#if defined(CFG_LOGGER_SIMPLE_LOGGER) && !defined(CFG_LOGGER_ADV_LOGGER)
	&stdio_logger,
#endif /* CFG_LOGGER_SIMPLE_LOGGER && !CFG_LOGGER_ADV_LOGGER */
	NULL,
};
#else /* CFG_LOGGER_EXTERNAL_DRIVER_CONF */
extern struct logger_driver_t *adrivers[];
#endif /* CFG_LOGGER_EXTERNAL_DRIVER_CONF */

struct log_level_t _log_levels[] = {
	{ LOG_LVL_DEBUG, "DEBUG", MAGENTA, 0 },
	{ LOG_LVL_INFO,	 "INFO",  BLUE,	   0 },
	{ LOG_LVL_OK,	 "OKAY",  GREEN,   0 },
	{ LOG_LVL_WARN,	 "WARN",  YELLOW,  0 },
	{ LOG_LVL_ERROR, "ERROR", RED,	   0 },
	{ LOG_LVL_RAW,	 "RAW",	  RESET,   0 },
};

static int _current_loglvl = LOG_LVL_EXTRA;

const char *_basename(const char *filename)
{
	size_t last_index = 0;

	for (size_t i = 0; i < strlen(filename); i++) {
		if (filename[i] == '/') {
			last_index = i;
		}
	}
	return &filename[last_index + 1];
}

int logger_mask2id(int mask)
{
	int n = 1;
	int i = 0;

	while (!(mask & n)) {
		n = n << 1;
		i++;
	}
	return i;
}

inline int logger_init()
{
	for (int i = 0; adrivers[i] != NULL; i++) {
		if (adrivers[i]->enabled && adrivers[i]->ops) {
			if (adrivers[i]->ops->init) {
				int error = adrivers[i]->ops->init(
					(void *)adrivers[i]);
				if (error < 0) {
					return -1;
				}
			}
		}
	}
	return 0;
}

int logger_get_loglvl()
{
	return _current_loglvl;
}

void logger_set_loglvl(int loglvl)
{
	LOG_INFO("Changing log level");
	_current_loglvl = loglvl;
}

void logger_log(const int lvl, const char *file, const char *fn, const int ln,
		char *fmt, ...)
{
	va_list va;

	struct line_info_t linfo = {
		.lvl	= lvl,
		.file	= _basename(file),
		.fn	= fn,
		.ln	= ln,
	};

	if (!(lvl & _current_loglvl)) {
		return;
	}

	va_start(va, fmt);
	for (int i = 0; adrivers[i] != NULL; i++) {
		if (adrivers[i]->enabled && adrivers[i]->ops) {
			if (adrivers[i]->ops->write) {
				adrivers[i]->ops->write(
					(void *)adrivers[i],
					&linfo, fmt, &va);
			}
			if (adrivers[i]->ops->flush) {
				adrivers[i]->ops->flush(
					(void *)adrivers[i]);
			}
		}
	}
	va_end(va);

	_log_levels[logger_mask2id(lvl)].counter++;
}

void logger_close()
{
	for (int i = 0; adrivers[i] != NULL; i++) {
		if (adrivers[i]->enabled && adrivers[i]->ops) {
			if (adrivers[i]->ops->close) {
				adrivers[i]->ops->close((void *)adrivers[i]);
			}
		}
	}
}
