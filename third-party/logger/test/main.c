#include <stdio.h>
#include <stdlib.h>

#include "logger-wrapper.h"

int main(int argc, char **argv)
{
	LOG_DEBUG("Test");
	LOG_INFO("Test");
	LOG_ERROR("Test");
	LOG_OK("Test");
	LOG_TRACE("Test");
	LOG_WARN("Test");

#ifndef CFG_LOGGER_DEEP_EMBEDDED
	logger_enable_file_logging("system");
	LOG_DEBUG("Test");
	LOG_INFO("Test");
	LOG_ERROR("Test");
	LOG_OK("Test");
	LOG_TRACE("Test");
	LOG_WARN("Test");

	logger_disable_file_logging();
	LOG_DEBUG("Tes");
	LOG_INFO("Tes");
	LOG_ERROR("Tes");
	LOG_OK("Tes");
	LOG_TRACE("Tes");
	LOG_WARN("Tes");
	LOG_WARN("Tes");

	logger_print_stats();

	logger_enable_threaded_mode();

	logger_enable_file_logging("system_threaded");
	LOG_DEBUG("Threaded Test");
	LOG_INFO("Threaded Test");
	LOG_ERROR("Threaded Test");
	LOG_OK("Threaded Test");
	LOG_TRACE("Threaded Test");
	LOG_WARN("Threaded Test");

	logger_disable_file_logging();
	logger_disable_threaded_mode();

	logger_enable_log_filter("DEBUG");
	LOG_INFO("Filter Test");
	LOG_ERROR("Filter Test");
	LOG_OK("Try Filter Test");
	LOG_TRACE("Filter Test");
	LOG_WARN("Try filter Test");

	logger_enable_file_logging("system_threaded-10M");
	/* for(;;) { */
	/* LOG_ERROR("Test"); */
	/* } */

	logger_exit();
#endif /* CFG_LOGGER_DEEP_EMBEDDED */
}
