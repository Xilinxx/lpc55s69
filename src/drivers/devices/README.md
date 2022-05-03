# Zeus300S Device drivers

* [Intro](#intro)
* [Drivers](#drivers)
  + [USB5807C](#usb5807c)
  + [TMDS181](#tmds181)

# Intro
The green power MCU communicates with multiple dedicated devices. All drivers for these devices can be found in `drivers/devices/`. These drivers implement the devices specific layers only. Protocol implementations can be found in `drivers/interfaces`.
# Drivers
## USB5807C
The USB5807C is a 7 port USB 3.0 Hub controlable via SMBus and SPI. Files with regards to the usb5807c are: `usb5807c.c`, `usb5807c.h` and `usb5807c-regmap.h`.

The USB5807C hub has an interesting I2C implementation. It doesn't work with standard register read/writes. Instead, one must load a string of bytes into the hub's internal memory and then send an memory execution command.

### Writing data
So writing some data to the hub is done using the following sequence:
- I2C Write with address 0x0 (Start of HUB RAM) and data containing a write command, data count, 16bit register and then the data.
- The run a memory execution command by performing an I2C Write with register address `0x9937`

### Reading data
Reading data follows the same steps as writing with one extra step at the end.
- I2C Write with address 0x0 (Start of HUB RAM) and data containing a read command, data count, 16bit register.
- The run a memory execution command by performing an I2C Write with register address `0x9937`
- I2C Read command with length equal to the data count specified in the initial write and address same as specified in the write command so 0x0 (Start of RAM).

### Particularities
- If the hub is configured to use the SMBus interfaces (via pinstrapping), it will not boot to runtime mode as standard. The hub stops in configuration mode and must explicitly be set runtime mode via the attach via smbus mode. This is done in the same way as the memory execution command. One must perform a write function with address `0xAA56`. More details can be found in `usb5807c.c`.

## TMDS181

TMDS181 is an HDMI retimer. The main reason for the GPMCU to communicate with this chip is to detect if there is active video. If active video is present, the system must be taken out of deep sleep.

### Power state modes
| State | Description |
|---|---|
| 0x00 | System has active video |
| 0x01 | System is connected and powered but no video is present |
| 0x02 | System has no active video |
