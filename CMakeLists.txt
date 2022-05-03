cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

# set(CMAKE_VERBOSE_MAKEFILE ON)

project(s300-zeus-gpmcu LANGUAGES C)

set(CMAKE_GENERATOR "Ninja")
message("Generator is set to ${CMAKE_GENERATOR}")

include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/helpers.cmake)

# Must be included at the beginning to take correct compiler

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# OPS Logger
add_subdirectory(third-party/logger/)

# Drivers
add_subdirectory(src/drivers)

include_directories(
	include
	boards/${BOARD_NAME}
	vendor/CMSIS
	vendor/device
	vendor/drivers
	src/application
	src/drivers/interfaces
	src/drivers/devices
	third-party/logger/include
	)

# compile the tools if aarch64 or x86
if(NOT ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
	# Set the name of the main executable
	set(MAIN_PROJECT_NAME "flash_tool" CACHE INTERNAL "")
	add_subdirectory(src/tools)
	# compile the program
else()
	message("-- Building target code for arm")
	# Supported target list
	set(TARGETS zeus300s gaia300d)

	# Set a default if nothing is given. Default to zeus300s
	if (NOT DEFINED BOARD_NAME)
		set(BOARD_NAME "zeus300s")
	endif()

	if (NOT BOARD_NAME IN_LIST TARGETS)
		message(FATAL_ERROR "Invalid target. Possible targets:${TARGETS}")
	endif()

	if (DEFINED SEMIHOSTING)
		set(SEMIHOSTING_FLAGS
			-DENABLE_SEMIHOSTING
			-DCFG_LOGGER_SIMPLE_LOGGER
			)
	endif()


	# Set exec name
	set(CMAKE_EXECUTABLE_SUFFIX ".elf")

	set(LINKER_SCRIPT_F0 ${PROJECT_SOURCE_DIR}/boards/startup/flash0.ld)
	set(LINKER_SCRIPT_F1 ${PROJECT_SOURCE_DIR}/boards/startup/flash1.ld)
	set(LINKER_SCRIPT_BTL ${PROJECT_SOURCE_DIR}/boards/startup/bootloader.ld)

	# Copy linker files
	file(COPY ${LINKER_SCRIPT_F0} DESTINATION ${PROJECT_BINARY_DIR})
	file(COPY ${LINKER_SCRIPT_F1} DESTINATION ${PROJECT_BINARY_DIR})
	file(COPY ${LINKER_SCRIPT_BTL} DESTINATION ${PROJECT_BINARY_DIR})
	file(COPY ${PROJECT_SOURCE_DIR}/boards/startup/memmap.ld DESTINATION ${PROJECT_BINARY_DIR})

	# define CPU OPTIONS
	set(CPU_OPTIONS
		-mcpu=cortex-m33
		-mthumb
		-mfpu=fpv5-sp-d16
		-mfloat-abi=hard
		)

	set(COMMON_LINK_OPTIONS
		-flto
		-fno-exceptions
		-fno-common
		-ffunction-sections
		-fdata-sections

		-nostartfiles

		-static

		--specs=nosys.specs
		--specs=nano.specs
		--specs=rdimon.specs

		-Wl,--print-memory-usage

		-Wl,--gc-sections
		)

	set(ARM_COMPILE_OPTS
		${CPU_OPTIONS}
		-std=c11
		-g3
		-DDEBUG

		-fstrict-volatile-bitfields
		-ffunction-sections
		-fno-common
		-fdata-sections
		-ffreestanding
		-fno-builtin
		-fmerge-constants
		-fstack-usage
		-ffast-math
		-fdelete-null-pointer-checks

		-Wall
		-Wextra
		-Wcast-align
		-Wconversion
		-Wsign-conversion
		-Wshadow
		-Wlogical-op
		-Wsuggest-final-types
		-Wsuggest-final-methods

		-DCPU_LPC55S69JBD100
		-DCPU_LPC55S69JBD100_cm33
		-DCPU_LPC55S69JBD100_cm33_core0
		-DCR_INTEGER_PRINTF
		-DPRINTF_FLOAT_ENABLE=0
		-D__USE_CMSIS
		-D__NEWLIB__

		-DCFG_LOGGER_EXTERNAL_DRIVER_CONF

		${SEMIHOSTING_FLAGS}
		)


	# NXP Drivers + ARM CMSIS + Startup files
	add_subdirectory(vendor/CMSIS)
	add_subdirectory(vendor/device)
	add_subdirectory(vendor/drivers)

	# BARCO Sources
	add_subdirectory(boards/${BOARD_NAME})
	add_subdirectory(src/application)
	add_subdirectory(src/bootloader)



	# Tests / Prototypes
	enable_testing()
	add_subdirectory(tests/native)

	find_library(POWER_HARDABI power_hardabi ${PROJECT_SOURCE_DIR}/vendor/libs)

	add_custom_target(gen_version
		COMMAND cd ${CMAKE_SOURCE_DIR} && ${CMAKE_SOURCE_DIR}/scripts/gen_version.sh)

	# Build bootloader
	set(BTLNAME "bootloader")
	set(MAIN_PROJECT_NAME "${BTLNAME}" CACHE INTERNAL "")
	add_executable(${BTLNAME}
		${VENDOR_DEVICE_SOURCES}
		${VENDOR_DRIVER_SOURCES}
		${GPMCU_DEVICE_SOURCES}
		${GPMCU_DEVICE_DRIVER_SOURCES}
		${GPMCU_INTERFACE_DRIVER_SOURCES}
		${GPMCU_STARTUP_SOURCES}
		${GPMCU_BOARD_SOURCES}
		${COMM_GEN_PROTO_SOURCES}
		${GPMCU_BOOTLOADER_SOURCES}
		${LOGGER_SRC}
		)

	add_dependencies(${BTLNAME} gen_version)

	target_compile_options(${BTLNAME}
		PRIVATE
		${ARM_COMPILE_OPTS}
		-DCOMM_DRIVER_UART
		)

	target_link_options(${BTLNAME}
		PRIVATE
		${CPU_OPTIONS}
		${COMMON_LINK_OPTIONS}
		-Wl,-Map=${BTLNAME}.map
		-T${LINKER_SCRIPT_BTL}
		)

	target_link_libraries(${BTLNAME} -Wl,--start-group)
	target_link_libraries(${BTLNAME} ${POWER_HARDABI})
	if (DEFINED SEMIHOSTING)
		target_link_libraries(${BTLNAME} rdimon)
	endif()
	target_link_libraries(${BTLNAME} -Wl,--end-group)

	add_custom_command(TARGET ${BTLNAME}
		POST_BUILD
		COMMENT "This command will be executed after building ${BTLNAME}"
		COMMAND ${OBJCOPY} -O binary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BTLNAME}.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BTLNAME}.bin
		COMMAND truncate -s 96k ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.bin
		VERBATIM
		)

	# Build Application for FLASH 0
	set(APPNAME "app-flash0")
	add_executable(${APPNAME}
		${VENDOR_DRIVER_SOURCES}
		${VENDOR_DEVICE_SOURCES}
		${GPMCU_DEVICE_DRIVER_SOURCES}
		${GPMCU_INTERFACE_DRIVER_SOURCES}
		${GPMCU_STARTUP_SOURCES}
		${GPMCU_BOARD_SOURCES}
		${GPMCU_APP_SOURCES}
		${LOGGER_SRC}
		)

	add_dependencies(${APPNAME} gen_version)

	set_source_files_properties(${VENDOR_DRIVER_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-unused-function")

	target_compile_options(${APPNAME}
		PRIVATE
		${ARM_COMPILE_OPTS}
		-DIS_APPLICATION
		-DSDK_I2C_BASED_COMPONENT_USED
		)

	target_link_options(${APPNAME}
		PRIVATE
		${CPU_OPTIONS}
		${COMMON_LINK_OPTIONS}
		-Wl,-Map=${APPNAME}.map
		-T${LINKER_SCRIPT_F0}
		)

	target_link_libraries(${APPNAME} -Wl,--start-group)
	target_link_libraries(${APPNAME} ${POWER_HARDABI})
	if (DEFINED SEMIHOSTING)
		target_link_libraries(${APPNAME} rdimon)
	endif()
	target_link_libraries(${APPNAME} -Wl,--end-group)

	add_custom_command(TARGET ${APPNAME}
		POST_BUILD
		COMMENT "This command will be executed after building ${APPNAME}"
		COMMAND ${OBJCOPY} -O binary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.bin
		COMMAND truncate -s 251k ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.bin
		VERBATIM
		)


	# Build Application for FLASH 1
	set(APPNAME "app-flash1")
	add_executable(${APPNAME}
		${VENDOR_DRIVER_SOURCES}
		${VENDOR_DEVICE_SOURCES}
		${GPMCU_DEVICE_DRIVER_SOURCES}
		${GPMCU_INTERFACE_DRIVER_SOURCES}
		${GPMCU_STARTUP_SOURCES}
		${GPMCU_BOARD_SOURCES}
		${GPMCU_APP_SOURCES}
		${LOGGER_SRC}
		)

	add_dependencies(${APPNAME} gen_version)

	set_source_files_properties(${VENDOR_DRIVER_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-unused-function")

	target_compile_options(${APPNAME}
		PRIVATE
		${ARM_COMPILE_OPTS}
		-DIS_APPLICATION
		-DSDK_I2C_BASED_COMPONENT_USED
		)

	target_link_options(${APPNAME}
		PRIVATE
		${CPU_OPTIONS}
		${COMMON_LINK_OPTIONS}
		-Wl,-Map=${APPNAME}.map
		-T${LINKER_SCRIPT_F1}
		)

	target_link_libraries(${APPNAME} -Wl,--start-group)
	target_link_libraries(${APPNAME} ${POWER_HARDABI})
	if (DEFINED SEMIHOSTING)
		target_link_libraries(${APPNAME} rdimon)
	endif()
	target_link_libraries(${APPNAME} -Wl,--end-group)

	add_custom_command(TARGET ${APPNAME}
		POST_BUILD
		COMMENT "This command will be executed after building ${APPNAME}"
		COMMAND ${OBJCOPY} -O binary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.bin
		COMMAND truncate -s 251k ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APPNAME}.bin
		VERBATIM
		)

endif()