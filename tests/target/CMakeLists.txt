# Copy linker files
# TODO: This is not great. Probably there is a better way to do this..

# Copy linker files
file(COPY ${PROJECT_SOURCE_DIR}/boards/startup/memmap.ld DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable("i2c-hub-poc"
	${VENDOR_DRIVER_SOURCES}
	${VENDOR_DEVICE_SOURCES}
	${GPMCU_INTERFACE_DRIVER_SOURCES}
	${GPMCU_DEVICE_DRIVER_SOURCES}
	${GPMCU_STARTUP_SOURCES}
	${GPMCU_BOARD_SOURCES}
	${LOGGER_SRC}
	${CMAKE_CURRENT_LIST_DIR}/logger_conf.c
	${CMAKE_CURRENT_LIST_DIR}/i2c_hub_poc.c
	)

find_library(POWER_HARDABI power_hardabi ${PROJECT_SOURCE_DIR}/vendor/libs)

target_compile_options("i2c-hub-poc"
	PRIVATE
	${ARM_COMPILE_OPTS}
	)

target_link_options("i2c-hub-poc"
	PRIVATE
	${CPU_OPTIONS}
	${COMMON_LINK_OPTIONS}
	-Wl,-Map=${APPNAME}.map
	-T${LINKER_SCRIPT_BTL}
	)

target_link_libraries("i2c-hub-poc" -Wl,--start-group)
target_link_libraries("i2c-hub-poc" ${POWER_HARDABI})
if (DEFINED SEMIHOSTING)
	target_link_libraries("i2c-hub-poc" rdimon)
endif()
target_link_libraries("i2c-hub-poc" -Wl,--end-group)
