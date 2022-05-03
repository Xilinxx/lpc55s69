# Todo list for GPMCU Code (05/12/2021)

- [Todo list for GPMCU Code (05/12/2021)](#todo-list-for-gpmcu-code-05122021)
  - [Bootloader](#bootloader)
  - [Application](#application)
  - [Drivers](#drivers)
  - [Others](#others)

## Bootloader

- [X] Save and restore vectortable in startup code
- [ ] SPI Mux configuration

- [ ] Finalize partition swapping logic
  - [ ] Retrieve errorcodes from application

## Application

- [x] GPMCU Application
- [x] Power state configuration
- [x] GoWIN UART Protocol
- [x] MainCPU(Zynq) UART Protocol
- [x] Store status for bootloader readback
- [x] i2c slave for MainCPU access
- [x] GoWIN SPI Flash integration (SPI-master)
- [ ] Read-back of log towards Zynq
- [x] SPI Mux configuration

## Drivers

- [ ] JogDial / Touch interface / ?
- [ ] Proximity sensor

## Others

- [x] Pinout configuration for Zeus300s board
- [x] Pinout configuration for Gaia300d board
