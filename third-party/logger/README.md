# OPS Logger

## Intro

The OPS logger is design with simplicity, scalability and minimalistic design in mind. With the introduction of V3 of the logger, a system of modular logging drivers has been added.

## Design

There are 2 main parts of the logger. Firstly, we have the core logger which can be found in `include/logger.h` and `src/logger.c`. These are the generic defines, prototypes and function calls which are by the user.
Secondly we have the drivers. These are the what will actually perform the "logging" function. Currently 2 drivers are defined. The simple logger, which can be found in `src/logger-stdio.c`. This driver just routes `LOG_*` calls to the printf function.
The second logger can be found in `drivers/logger-nxp-uart.c` and wraps the `LOG_*` calls with NXP's `USART_*` calls.

One can either use the standard setup where printf is used by add the `-DCFG_LOGGER_SIMPLE_LOGGER` to the compilation flags. Or define an external logger config by setting `-DCFG_LOGGER_EXTERNAL_DRIVER_CONF`.

> Note: `-DCFG_LOGGER_EXTERNAL_DRIVER_CONF` and `-DCFG_LOGGER_SIMPLE_LOGGER` can be used together for example in an embedded system where semihosting is available and printf is routed to the debugger via semihosting.

The logger supports simultaneous usage of loggers. This can be interesting when there is debug output but also file or memory logging done.

Adding an external logger config, one need to include a `logger-config.c` which looks something like this:
```c
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

struct logger_driver_t *adrivers[] = {
#if defined(CFG_LOGGER_SIMPLE_LOGGER) && !defined(CFG_LOGGER_ADV_LOGGER)
	&stdio_logger,
#endif /* CFG_LOGGER_SIMPLE_LOGGER && !CFG_LOGGER_ADV_LOGGER */
	&uart_logger,
	NULL,
};
```
