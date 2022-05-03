file build_zeus/bootloader.elf
# file ./app.elf
# file ./tests/target/i2c-hub-poc.elf
tar ext:2331
monitor halt
monitor reset 0
load build_zeus/bootloader.elf
# load ./app.elf
# load ./tests/target/i2c-hub-poc.elf

monitor semihosting enable
