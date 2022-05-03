# Copy linker files
# TODO: This is not great. Probably there is a better way to do this..

set(CMAKE_C_COMPILER "gcc")

# Set exec name
set(CMAKE_EXECUTABLE_SUFFIX "")

add_executable("comm_test"
	${LOGGER_NATIVE_SRC}
	${COMM_GEN_PROTO_SOURCES}
	${CMAKE_SOURCE_DIR}/src/tools/crc.c
	${CMAKE_CURRENT_LIST_DIR}/socket.c
	${CMAKE_CURRENT_LIST_DIR}/comm_tcp.c
	${CMAKE_CURRENT_LIST_DIR}/comm_test.c
	${CMAKE_CURRENT_LIST_DIR}/storage_linux_file.c
	)

include_directories(${CMAKE_SOURCE_DIR}/src/tools)

target_compile_options("comm_test"
	PRIVATE
	-DCFG_LOGGER_SIMPLE_LOGGER
	-DCOMM_DRIVER_TCP)

add_executable("unit_comm_test"
	${LOGGER_NATIVE_SRC}
	${COMM_GEN_PROTO_SOURCES}
	${CMAKE_SOURCE_DIR}/src/tools/crc.c
	${CMAKE_CURRENT_LIST_DIR}/socket.c
	${CMAKE_CURRENT_LIST_DIR}/comm_unit.c
	${CMAKE_CURRENT_LIST_DIR}/unit_test_comm.c
	)

target_compile_options("unit_comm_test"
	PRIVATE
	-Og
	-ggdb
	-DCFG_LOGGER_SIMPLE_LOGGER
	-DCOMM_DRIVER_UNIT
	-DUNIT_TEST
	)

target_link_libraries("unit_comm_test"
	-lcmocka
	)

add_test(NAME "unit_comm_test" COMMAND unit_comm_test
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})