# Changelog

## 1.0 (2026-11-24)

Fixes:

 - Pressing Enter at the main prompt no longer reports an unknown command

Features:
 - Added pin monitor (`m`) showing each channel's state, edge count, frequency and duty cycle
 - Added passive UART TX and baud rate scan (`u`)
 - Added I2C bus scan (`i`)
 - Added SPI flash JEDEC ID reader (`f`)
 - Added firmware for the Waveshare RP2040-PiZero, with header pin numbers in the console
 - Added I2C EEPROM dump (`e`): read 24C01 to 24C512 chips, view them, and send them to the computer as Intel HEX. Dumps are never saved on PROBINIX.
 - Added fast JTAG IDCODE scan (`d`) that finds TCK, TMS and TDO, then TDI with a BYPASS test
 - Added JTAG RTCK detection (`r`)
 - Added manual pin control (`g`) to read, drive, release, pulse and watch GP0 to GP15

Others:
 - First PROBINIX release, includes code by Aodrulez (see LICENSE)
