set(GPMCU_INTERFACE_DRIVER_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/interfaces/logger_uart.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/logger_mem.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/storage_flash.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/storage_spi_flash.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/commp_uart.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/crc.c
	PARENT_SCOPE
)

set (COMM_GEN_PROTO_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/interfaces/commp.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/commp_parser.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/commp_cmd_helpers.c
	${CMAKE_CURRENT_LIST_DIR}/interfaces/commp_opcode_helpers.c
	PARENT_SCOPE
	)

set(GPMCU_DEVICE_DRIVER_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/devices/usb5807c.c
	${CMAKE_CURRENT_LIST_DIR}/devices/usb7206c.c
	${CMAKE_CURRENT_LIST_DIR}/devices/tmds181.c
	${CMAKE_CURRENT_LIST_DIR}/devices/si5341.c
	${CMAKE_CURRENT_LIST_DIR}/devices/is25xp.c
	PARENT_SCOPE
)
