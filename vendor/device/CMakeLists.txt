set(VENDOR_DEVICE_HEADERS
	fsl_device_registers.h
	LPC55S69_cm33_core0_features.h
	LPC55S69_cm33_core0.h
	system_LPC55S69_cm33_core0.h
)

set(VENDOR_DEVICE_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/system_LPC55S69_cm33_core0.c
	PARENT_SCOPE
)
