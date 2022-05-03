#include "logger.h"

void longer_function_name()
{
	LOG_DEBUG("Long function name test");
}

int main()
{
	logger_init();
	LOG_DEBUG("Test");
	LOG_INFO("Test");
	LOG_OK("Test");
	LOG_WARN("Test");
	LOG_ERROR("Test");
	longer_function_name();
	logger_close();
}
