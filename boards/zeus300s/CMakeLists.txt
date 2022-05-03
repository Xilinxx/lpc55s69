set(GPMCU_BOARD_HEADERS
	board.h
	clock_config.h
	peripherals.h
	pin_mux.h
)

set(GPMCU_BOARD_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/board.c
	${CMAKE_CURRENT_LIST_DIR}/clock_config.c
	${CMAKE_CURRENT_LIST_DIR}/peripherals.c
	${CMAKE_CURRENT_LIST_DIR}/pin_mux.c
	PARENT_SCOPE
)
