set(CMAKE_C_COMPILER "gcc")
# Find cmocka
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/../test/cmocka/include)
FIND_LIBRARY(CMOCKA_LIBRARY cmocka HINTS ../../test/cmocka/build/src)

# Include code coverage
#set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/scripts/cmake)
#INCLUDE(CodeCoverage)

# Set exec name
set(CMAKE_EXECUTABLE_SUFFIX "")

include_directories(${CMAKE_SOURCE_DIR}/src/tools)
include_directories(${CMAKE_SOURCE_DIR}/src/application)

### unit_comm_protocol_test ###
set(MYTEST "unit_comm_protocol_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_SOURCE_DIR}/src/application/comm/protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/comm.c
  ${CMAKE_SOURCE_DIR}/src/application/run/comm_run.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_helper.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_mock.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--wrap=PROTO_TX_SendMsg
  -Wl,--wrap=spi_queue_msg_param
  -Wl,--defsym,switch_boot_partition=__wrap_switch_boot_partition
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

### unit_comm_protocol_array_test ###
set(MYTEST "unit_comm_protocol_array_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_SOURCE_DIR}/src/application/comm/protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/comm.c
  ${CMAKE_SOURCE_DIR}/src/application/run/comm_run.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_mock.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_helper.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_array.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--wrap=PROTO_TX_SendMsg
  -Wl,--wrap=spi_queue_msg_param
  # -Wl,--wrap=sharedram_log_read  # <- not working, replace by,
  -Wl,--defsym,sharedram_log_read=__wrap_sharedram_log_read
  -Wl,--defsym,switch_boot_partition=__wrap_switch_boot_partition
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

### unit_comm_protocol_bootcode_test ###
set(MYTEST "unit_comm_protocol_bootcode_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_SOURCE_DIR}/src/application/comm/protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/comm.c
  ${CMAKE_SOURCE_DIR}/src/application/run/comm_run.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_helper.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_mock.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_comm_protocol_bootcode.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--wrap=PROTO_TX_SendMsg
  -Wl,--wrap=spi_queue_msg_param
  -Wl,--defsym,switch_boot_partition=__wrap_switch_boot_partition
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

### unit_comm_gowin_protocol_test ###
set(MYTEST "unit_comm_gowin_protocol_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol_mock.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--defsym,GOWIN_TX_SendMsg=__wrap_GOWIN_TX_SendMsg
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

### unit_comm_gowin_protocol_queue_test ###
set(MYTEST "unit_comm_gowin_protocol_queue_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol_mock.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol_queue.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--defsym,GOWIN_TX_SendMsg=__wrap_GOWIN_TX_SendMsg
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

### unit_comm_gowin_protocol_byte_test ###
set(MYTEST "unit_comm_gowin_protocol_byte_test")
add_executable(${MYTEST}
  ${LOGGER_NATIVE_SRC}
  ${COMM_GEN_PROTO_SOURCES}
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol_mock.c
  ${CMAKE_SOURCE_DIR}/src/application/comm/gowin_protocol.c
  ${CMAKE_SOURCE_DIR}/src/application/data_map.c
  ${CMAKE_CURRENT_LIST_DIR}/unit_test_gowin_protocol_byte.c
  )

target_compile_options(${MYTEST}
  PRIVATE
  -Og
  -DCFG_LOGGER_SIMPLE_LOGGER
  -DUNIT_TEST
  )

target_link_libraries(${MYTEST}
  -Wl,--allow-multiple-definition
  -Wl,--defsym,GOWIN_TX_SendMsg=__wrap_GOWIN_TX_SendMsg
  -lcmocka
  )

add_test(NAME ${MYTEST} COMMAND ${MYTEST}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
