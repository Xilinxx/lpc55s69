/**
 * @file logger.h
 * @brief Main include file for logger
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v3.0
 * @date 2020-08-10
 */

#ifndef _LOGGER_V3_H_
#define _LOGGER_V3_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include "colors.h"

#define LOG_LVL_DEBUG           0x00000001      //!< Debugging
#define LOG_LVL_INFO            0x00000002      //!< Info
#define LOG_LVL_OK              0x00000004      //!< Success
#define LOG_LVL_WARN            0x00000008      //!< Warning
#define LOG_LVL_ERROR           0x00000010      //!< Error
#define LOG_LVL_RAW             0x10000000      //!< Raw loggin

/** All standard Logging */
#define LOG_LVL_ALL (LOG_LVL_INFO | LOG_LVL_WARN | LOG_LVL_ERROR | LOG_LVL_OK | \
		     LOG_LVL_RAW)

/** Acceptable level of logging for production software */
#define LOG_LVL_PRODUCTION (LOG_LVL_OK | LOG_LVL_WARN | LOG_LVL_ERROR | \
			    LOG_LVL_RAW)

/** Extra debugging information */
#define LOG_LVL_EXTRA (LOG_LVL_ALL | LOG_LVL_DEBUG | LOG_LVL_RAW)

/** No logging at all */
#define LOG_LVL_NONE 0

/** Max logger name */
#define LOGGER_DRV_NAME 16

/** Max string length */
#define MAX_STR_LEN 256

struct line_info_t {
	const int	lvl;    //!< Log level
	const char *	file;   //!< File string
	const char *	fn;     //!< Function name
	const int	ln;     //!< Line number
};

/** Init driver callback */
typedef int (*init_fn)(void *drv);

/** Write callback function */
typedef int (*write_fn)(void *drv, struct line_info_t *linfo, char *fmt, va_list *v);

/** Read callback function */
typedef int (*read_fn)(void *drv, char *buffer);

/** Flush callback function */
typedef int (*flush_fn)(void *drv);

/** Close callback function */
typedef void (*close_fn)(void *drv);

/** Logger operation struct */
struct logger_ops_t {
	init_fn		init;   //!< Init driver
	write_fn	write;  //!< Write function for driver
	read_fn		read;   //!< Read function for driver
	flush_fn	flush;  //!< Flush function for driver
	close_fn	close;  //!< Close function for driver
};

/** Logger driver structure */
struct logger_driver_t {
	bool				enabled;                //!< Enable the logger
	char				name[LOGGER_DRV_NAME];  //!< Driver name
	const struct logger_ops_t *	ops;                    //!< Logger operations
	void *				priv_data;              //!< private driver data
};

/** brief  Log level definition */
struct log_level_t {
	int		mask;           //!< Mask associated with the log level
	const char *	name;           //!< Log level name. (What will be printed before msg)
	const char *	color;          //!< Color for the name
	int		counter;        //!< Internal counter for number of messages
};

extern struct log_level_t _log_levels[]; //!< Log levels

/**
 * @brief  Convert logger mask to id in _log_levels array
 *
 * @param mask Log level mask
 *
 * @returns log level id
 */
int logger_mask2id(int mask);

/**
 * @brief  Initial the logger
 *
 * @returns -1 if failed
 */
int logger_init();

/**
 * @brief  Set a new log level
 *
 * @param loglvl The level that will be set
 */
void logger_set_loglvl(int loglvl);


/**
 * @brief  Get the current log level
 *
 * @returns   returns the current log level
 */
int logger_get_loglvl();

/**
 * @brief  Close the logger
 */
void logger_close();

/**
 * @brief  Write to the logger(s)
 *
 * @param lvl Log level
 * @param file Current file name
 * @param fn Current function name
 * @param ln Current line number
 * @param fmt string va format
 * @param ... va_args
 */
void logger_log(const int lvl, const char *file, const char *fn, const int ln, char *fmt, ...);

#define LOG_DEBUG(msg, ...) \
	logger_log(LOG_LVL_DEBUG, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#define LOG_INFO(msg, ...) \
	logger_log(LOG_LVL_INFO, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#define LOG_OK(msg, ...) \
	logger_log(LOG_LVL_OK, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#define LOG_WARN(msg, ...) \
	logger_log(LOG_LVL_WARN, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#define LOG_ERROR(msg, ...) \
	logger_log(LOG_LVL_ERROR, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#define LOG_RAW(msg, ...) \
	logger_log(LOG_LVL_RAW, __FILE__, __FUNCTION__, __LINE__, msg, \
		   ## __VA_ARGS__)

#endif /* _LOGGER_V3_H_ */
