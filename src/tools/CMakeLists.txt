# Copy linker files
# TODO: This is not great. Probably there is a better way to do this..

#set(CMAKE_C_COMPILER "gcc")

# Set exec name
set(CMAKE_EXECUTABLE_SUFFIX "")

message("-- Building ${MAIN_PROJECT_NAME}")

# for CMAKE_INSTALL_*
include(GNUInstallDirs)

add_executable(${MAIN_PROJECT_NAME}
	${LOGGER_NATIVE_SRC}
	${COMM_GEN_PROTO_SOURCES}
	${CMAKE_CURRENT_LIST_DIR}/crc.c
	${CMAKE_CURRENT_LIST_DIR}/comm_serial.c
	${CMAKE_CURRENT_LIST_DIR}/comm_udp.c
	${CMAKE_CURRENT_LIST_DIR}/comm_i2c_gpmcu.c
	${CMAKE_CURRENT_LIST_DIR}/logger_conf.c
	${CMAKE_CURRENT_LIST_DIR}/flash_tool.c
	${CMAKE_CURRENT_LIST_DIR}/socket.c
	)

target_compile_options(${MAIN_PROJECT_NAME}
	PRIVATE
	-Og
	-ggdb
	-DCFG_LOGGER_EXTERNAL_DRIVER_CONF
	-DCOMM_DRIVER_SERIAL)

target_link_libraries(${MAIN_PROJECT_NAME}
	i2c
	)

install(TARGETS ${MAIN_PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
