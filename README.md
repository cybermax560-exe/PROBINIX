<div align="center">

# PROBINIX

**A JTAGulator-inspired hardware hacking multi-tool for RP2040 boards**

![Version](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-RP2040-red)
![Boards](https://img.shields.io/badge/boards-Pico%20%7C%20Pico%20W%20%7C%20PiZero-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)

[Supported Boards](#supported-boards) •
[Quick Start](#quick-start) •
[Tool Guide](#tool-guide) •
[Wiring](#wiring-reference) •
[Troubleshooting](#troubleshooting)

</div>

---

## Overview

PROBINIX turns an inexpensive Raspberry Pi Pico or Pico W into a complete hardware hacking toolkit. Connect it to an unknown circuit board and PROBINIX helps you find the debug ports, serial consoles, buses and flash chips, then talk to them, all from a simple menu in any serial terminal.

<div align="center">

![PROBINIX interface](images/PROBINIX-Console-v1.0.png?raw=true "PROBINIX v1.0 interface")

</div>

| Discover | Investigate | Connect |
|---|---|---|
| Find JTAG and SWD debug pinouts | Monitor pin activity | USB-to-serial bridge |
| Find UART TX, RX and baud rate | Identify SPI flash chips | Flashrom SPI programmer |
| Identify chips by IDCODE | Dump I2C EEPROMs | OpenOCD JTAG and SWD adapter |
| Scan I2C buses | Send dumps to your computer | CMSIS-DAP debugger |

## Table of Contents

- [Supported Boards](#supported-boards)
- [Quick Start](#quick-start)
- [Using the Console](#using-the-console)
- [Command Reference](#command-reference)
- [Tool Guide](#tool-guide)
- [Wiring Reference](#wiring-reference)
- [Hardware Boot Modes](#hardware-boot-modes)
- [Waveshare RP2040-PiZero](#waveshare-rp2040-pizero)
- [Troubleshooting](#troubleshooting)
- [Safety Notes](#safety-notes)
- [Building from Source](#building-from-source)
- [Project Structure](#project-structure)
- [Acknowledgments](#acknowledgments)
- [License](#license)

## Supported Boards

PROBINIX runs on boards built around the **RP2040** microcontroller. One firmware file works on all of the boards below, except the Waveshare RP2040-PiZero, which has its own firmware file.

| Board | Status | Notes |
|---|---|---|
| **Raspberry Pi Pico** | ✅ Supported | Reference board for PROBINIX |
| **Raspberry Pi Pico H** | ✅ Supported | Same as Pico, with pre-soldered headers |
| **Raspberry Pi Pico W** | ✅ Supported | Wi-Fi and Bluetooth are not used. The onboard LED is not used as a status light. |
| **Raspberry Pi Pico WH** | ✅ Supported | Same as Pico W, with pre-soldered headers |
| **Waveshare RP2040-Zero** | ✅ Supported | Compact board. See the [pinout image](#wiring-reference). |
| **Waveshare RP2040-PiZero** | ✅ Supported | Uses its own firmware file. See [Waveshare RP2040-PiZero](#waveshare-rp2040-pizero). |
| Other RP2040 boards | ⚠️ Should work | The board must expose GP0 to GP15. GP26 to GP28 are needed only for boot modes. |
| RP2350 boards, such as Pico 2 | 🧪 Experimental | No firmware file in v1.0 |

> [!NOTE]
> On the Pico and Pico H, the onboard LED stays on while PROBINIX is ready and flickers during JTAG scans. The Pico W connects its LED through the wireless chip, so the LED stays off. Everything else works the same.

## Quick Start

### 1. Flash the firmware

1. Download the firmware for your board from the [`firmware`](firmware) folder or the [Releases](../../releases) page.
   - Raspberry Pi Pico, Pico W and other RP2040 boards: `PROBINIX-v1.0-RP2040.uf2`
   - Waveshare RP2040-PiZero: `PROBINIX-v1.0-RP2040-PiZero.uf2`
2. Hold the **BOOTSEL** button and connect the board to your computer with a USB cable.
3. Release the button when a drive named `RPI-RP2` appears.
4. Copy the `.uf2` file onto that drive. The board restarts as PROBINIX.

### 2. Open the console

PROBINIX appears as a USB serial port. The baud rate setting does not matter for the console.

| System | Port name | Example terminal command |
|---|---|---|
| Windows | `COM3`, `COM4` and so on (see Device Manager) | Open the port in [PuTTY](https://www.putty.org/) or [Tera Term](https://teratermproject.github.io/) |
| Linux | `/dev/ttyACM0` | `picocom -b 115200 /dev/ttyACM0` |
| macOS | `/dev/cu.usbmodem*` | `screen /dev/cu.usbmodem* 115200` |

Press any key to show the menu.

### 3. Connect a target

1. Connect **GND** on PROBINIX to **GND** on the target. Always do this first.
2. Connect the target's test points to **GP0, GP1, GP2** and so on, in order.
3. Choose a tool from the menu.

> [!TIP]
> Scans test channels in order, starting at GP0. Using the lowest channels first gives the fastest results.

## Using the Console

The console is designed to be read at a glance.

| Element | Meaning |
|---|---|
| Cyan title bar | Firmware version, chip, clock speed and the board's unique ID |
| `probinix>` | PROBINIX is waiting for a command |
| Yellow `?` | PROBINIX needs your input |
| Green `[+]` | Success or a result was found |
| Red `[-]` | Nothing was found or something failed |
| Gray text | Tips and notes |

Commands are single keys and are **case-sensitive**. For example, `u` runs the UART finder and `U` starts USB-to-serial mode. When asked for a number, type it and press Enter. Pressing Enter alone accepts the default shown in brackets.

> [!IMPORTANT]
> Turn off local echo in your terminal, or every key appears twice.

## Command Reference

| Key | Command | What it does |
|:---:|---|---|
| `h` | Help | Clears the screen and shows the menu |
| `v` | System info | Shows firmware version, author, processor, clock, board ID, build date and pin pulsing setting |
| `j` | JTAG scan | Finds TDI, TDO, TCK, TMS and TRST, then identifies the chips |
| `s` | SWD scan | Finds SWCLK and SWDIO, then identifies the chip |
| `d` | Fast IDCODE scan | Quickly finds TCK, TMS and TDO by reading the chip's ID, then finds TDI |
| `r` | RTCK detection | Finds the optional JTAG return-clock pin |
| `p` | Pin pulsing | Changes how idle channels are pulled during JTAG and SWD scans |
| `m` | Pin monitor | Shows each channel's state, edges, frequency and duty cycle |
| `u` | UART finder | Finds the target's TX pin and baud rate, then optionally its RX pin |
| `i` | I2C scanner | Lists the addresses of devices on an I2C bus |
| `f` | SPI flash ID | Reads the manufacturer and size of an SPI flash chip |
| `e` | I2C EEPROM dump | Reads 24Cxx EEPROMs and sends the dump to your computer |
| `g` | Manual pin control | Reads every channel, and drives, releases or pulses one channel by hand |
| `U` | USB-to-serial | Turns PROBINIX into a USB serial adapter |
| `F` | Flashrom | Turns PROBINIX into a Flashrom SPI programmer |
| `O` | OpenOCD | Turns PROBINIX into a JTAG and SWD adapter for OpenOCD |
| `C` | CMSIS-DAP | Turns PROBINIX into a CMSIS-DAP v1.0 debugger |

Commands in **uppercase** start a hardware mode. A hardware mode replaces the menu until you unplug or reset the board.

## Tool Guide

### Debug interface discovery

#### `j` JTAG scan

Finds a JTAG port on unlabeled test points.

1. Connect GND and the suspected JTAG test points to GP0 and up.
2. Press `j` and enter how many channels you connected, from 4 to 16.
3. PROBINIX tests every combination of TDI, TDO, TCK and TMS. A progress bar shows how far it has gone.
4. When it finds a working combination, it searches the remaining channels for TRST.

```text
  [+] JTAG pinout found
    TDI = GP2   TDO = GP3   TCK = GP0   TMS = GP1   TRST = not found
    Device 0   IDCODE 0x3BA00477   ARM Ltd, part 0xBA00, rev 3
```

PROBINIX lists every device in the JTAG chain with its IDCODE, manufacturer, part number and revision. It then offers to start OpenOCD JTAG mode using the pins it found.

> [!TIP]
> The number of combinations grows quickly with more channels. Connect only the test points you suspect.

#### `s` SWD scan

Finds an Arm SWD debug port.

1. Connect GND and the suspected SWD test points to GP0 and up.
2. Press `s` and enter how many channels you connected, from 2 to 16.
3. PROBINIX tries each pin pair as SWCLK and SWDIO and reads the chip's debug port ID.

```text
  [+] SWD pinout found
    SWDIO = GP1   SWCLK = GP0
    Device 0   IDCODE 0x2BA01477   ARM Ltd, part 0xBA01, rev 2
```

PROBINIX then offers to start OpenOCD SWD mode using the pins it found.

#### `d` Fast IDCODE scan

A faster way to find a JTAG port, based on JTAGulator's IDCODE scan. Most chips load their ID into the JTAG data register at reset, so PROBINIX can read it using only TCK, TMS and TDO. This skips TDI and tests far fewer combinations than the full scan.

1. Connect GND and the suspected JTAG test points to GP0 and up.
2. Press `d` and enter how many channels you connected, from 3 to 16.
3. PROBINIX reports every TCK, TMS and TDO combination that returns a valid ID, with the chip's manufacturer.
4. It then tests the remaining channels as TDI with a BYPASS test and reports the full pinout.

```text
  [+] TCK, TMS and TDO found
    TCK = GP0   TMS = GP1   TDO = GP3
    Device 0   IDCODE 0x4BA00477   ARM Ltd, part 0xBA00, rev 4

  [+] TDI = GP2 (confirmed with a BYPASS test, 1 device in the chain)
```

With all four pins found, PROBINIX offers to start OpenOCD JTAG mode.

| Channels | Full scan (`j`) combinations | Fast scan (`d`) combinations |
|---|---|---|
| 6 | 840 | 120 |
| 8 | 3,024 | 336 |
| 16 | 57,120 | 3,360 |

> [!TIP]
> Start with `d`. If it finds nothing, the chip may not support IDCODE, so use the full scan (`j`).

#### `r` RTCK detection

Some Arm chips have an extra JTAG pin called RTCK, which returns the clock to the debugger so it can match the chip's speed. Most targets don't have one.

1. Press `r` and enter how many channels to check.
2. Enter the TCK and TMS pins. If you've just run `j` or `d`, press Enter to use the pins it found.
3. PROBINIX holds TMS high so the chip stays in reset, toggles TCK 64 times, and reports any channel that follows every change.

#### `p` Pin pulsing

Changes how unused channels are pulled during JTAG and SWD scans. The menu shows the current setting, such as `Toggle pin pulsing [OFF]`. Try switching it if a scan finds nothing on a board you are sure has a debug port.

### Target reconnaissance

#### `m` Pin monitor

Shows what every channel is doing, without driving any pins.

1. Press `m` and enter how many channels to check. Press Enter for all 16.
2. PROBINIX tests each pin's state, then counts activity for 1 second.

| Column | Meaning |
|---|---|
| State | `High`, `Low`, `Floating` (not connected or not driven) or `Active` (changing) |
| Edges | Number of signal changes in 1 second |
| Approx Hz | Approximate frequency of an active signal |
| High | Percentage of time the signal was high |

Use it to spot clocks, data lines, idle UART lines (high) and unconnected pins before running other tools.

#### `u` UART finder

Finds a serial console in two steps.

**Step 1: TX and baud rate.** PROBINIX listens while the target boots and recognizes UART data from 1200 to 460800 baud.

1. Connect GND and the likely UART test points to GP0 and up.
2. Press `u` and enter how many channels you connected.
3. Press any key, then reset or power-cycle the target. PROBINIX listens for 5 seconds.

**Step 2: RX (optional).** After the target finishes booting, press `y`. PROBINIX sends Enter on each quiet channel and watches the target's TX pin for a reply.

```text
     [+] GP3 looks like target TX at 57600 baud
     [+] GP5 looks like target RX (the target replied)

     Wire target TX to GP1 and target RX to GP0, then use USB-to-serial mode (U) at 57600 baud.
```

> [!NOTE]
> Step 2 drives the other channels. Connect them only to likely UART test points, and skip it if a pin could be a reset or power-control line. It only works when the target replies to input, such as a login prompt or shell. If more than one pin replies, test each one.

#### `i` I2C scanner

Lists the addresses of devices on an I2C bus.

1. Press `i`, then enter the SDA and SCL pins. Press Enter to use the defaults, GP0 and GP1.
2. PROBINIX checks addresses `0x08` to `0x77` at about 50 kHz, using its internal pull-ups.

```text
         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    00:                         -- -- -- -- -- -- -- --
    50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
```

If a device is holding the bus low, PROBINIX first tries to free it, then warns you if it can't.

> [!NOTE]
> The scanner reads one byte from each device that answers. A few devices change state when read, so avoid scanning sensitive live systems.

#### `f` SPI flash ID

Identifies an SPI flash chip using the same wiring as Flashrom mode.

1. Wire the chip as shown in the [SPI wiring table](#spi-flash).
2. Press `f`, then any key.

```text
     JEDEC ID     : EF 40 18
     Manufacturer : Winbond Electronics
     Capacity     : 16 MB (typical encoding)
```

Use it to confirm your wiring before reading or writing the chip with Flashrom.

#### `e` I2C EEPROM dump

Reads 24Cxx serial EEPROMs, such as the configuration and calibration chips found on many boards, and sends the dump to your computer.

> [!NOTE]
> Dumps are **never saved on PROBINIX**. A dump stays in PROBINIX's memory only until you read another chip or unplug it. The tool only reads the EEPROM and never writes to it.

```text
     r  Read an EEPROM            v  View the last dump
     x  Send it to your computer  q  Back to the main menu
```

**Reading a chip**

1. Connect the EEPROM's **SDA** and **SCL** to any GPIO (GP0 and GP1 by default), **GND** to GND, and power the chip at **3.3 V**.
2. Press `e`, then `r`.
3. Choose the chip type, the SDA and SCL pins, and the value of the chip's A2-A0 address pins. Most boards tie these to GND, which is `0`.
4. PROBINIX reads the whole chip, then shows its size, CRC32 checksum and a hex dump, 256 bytes per page. Press Enter for the next page or `q` to stop.
5. Press `y` to send the dump to your computer, or press `x` later.

| Chip | Size | | Chip | Size |
|---|---|---|---|---|
| 24C01 | 128 bytes | | 24C32 | 4 KB |
| 24C02 | 256 bytes | | 24C64 | 8 KB |
| 24C04 | 512 bytes | | 24C128 | 16 KB |
| 24C08 | 1 KB | | 24C256 | 32 KB |
| 24C16 | 2 KB | | 24C512 | 64 KB |

> [!TIP]
> If you're not sure of the size, choose a larger type. A smaller chip repeats its data, so the point where the data starts repeating shows its real size.

**Saving the dump on your computer**

PROBINIX sends the dump as Intel HEX text through the serial console. Your terminal saves it to a file.

1. Turn on logging in your terminal. In PuTTY, use **Session > Logging > All session output**. In picocom, start it with `--logfile eeprom.log`.
2. Press `y` after reading, or `x` from the EEPROM menu, then press any key.
3. Stop logging when PROBINIX reports that the dump was sent, and compare the CRC32 shown with your file if needed.
4. Convert the log to a binary file:

**Linux**

```sh
grep '^:' eeprom.log > eeprom.hex
objcopy -I ihex -O binary eeprom.hex eeprom.bin
```

**Windows:** open the log in a hex editor such as [HxD](https://mh-nexus.de/en/hxd/) with **File > Import > Intel HEX**, then save it as a `.bin` file.

#### `g` Manual pin control

Reads and controls GP0 to GP15 by hand, like JTAGulator's GPIO mode. Use it to check wiring, find which test point a pin connects to, or hold a target's reset or boot pin.

```text
    GPIO    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    Mode   in  in  HI  in  in  in  in  in  in  in  in  in  in  in  in  in
    Level   0   1   1   0   0   0   0   0   0   0   0   0   0   0   0   0
```

| Key | Action |
|---|---|
| `r` | Read all pins again |
| `h` / `l` | Drive a pin high or low |
| `i` | Release a pin back to an input |
| `p` | Pulse a pin low for 100 ms, such as a reset line |
| `w` | Watch all levels live until you press a key |
| `q` | Release every pin and return to the main menu |

On the RP2040-PiZero, the table also shows each GPIO's header pin. All pins are released when you leave.

> [!WARNING]
> Drive only pins that are inputs on the target, such as reset or boot pins. Driving a pin that the target is also driving can damage both boards.

### Hardware modes

Each hardware mode shows its wiring and example commands, then asks you to press a key. PROBINIX then disconnects and reconnects over USB in the new mode. **Close your terminal program before using the port with another tool.** To leave a hardware mode, unplug or reset the board.

#### `U` USB-to-serial

Turns PROBINIX into a USB serial adapter for a target's console on GP0 (TX) and GP1 (RX). The baud rate, data bits, parity and stop bits come from your terminal's settings.

```sh
# Linux
picocom -b 57600 /dev/ttyACM0
```

On Windows, open the COM port in PuTTY or Tera Term at the target's baud rate.

#### `F` Flashrom SPI programmer

Reads and writes SPI flash chips with [Flashrom](https://www.flashrom.org/) using the serprog protocol.

**Windows** (replace `COM4` with your PROBINIX port)

```sh
flashrom -p serprog:dev=COM4:115200,spispeed=4M
flashrom -p serprog:dev=COM4:115200,spispeed=4M -r backup.bin
flashrom -p serprog:dev=COM4:115200,spispeed=4M -w firmware.bin
```

**Linux** (find the port with `ls /dev/ttyACM*`)

```sh
flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M
flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M -r backup.bin
flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M -w firmware.bin
```

The first command detects the chip, the second makes a backup, and the third writes a new image. Always keep a backup before writing. If reads fail, use `spispeed=1M`.

#### `O` OpenOCD adapter

Lets [OpenOCD](https://openocd.org/) debug a target over JTAG or SWD, using the Bus Pirate protocol.

**Windows**

```sh
openocd -f interface/buspirate.cfg -c "buspirate port COM4; transport select jtag;" -f target/esp32.cfg
openocd -f interface/buspirate.cfg -c "buspirate port COM4; transport select swd;" -f target/stm32f1x.cfg
```

**Linux**

```sh
openocd -f interface/buspirate.cfg -c "buspirate port /dev/ttyACM0; transport select jtag;" -f target/esp32.cfg
openocd -f interface/buspirate.cfg -c "buspirate port /dev/ttyACM0; transport select swd;" -f target/stm32f1x.cfg
```

Replace the target file with the one for your chip.

#### `C` CMSIS-DAP debugger

Turns PROBINIX into a standard CMSIS-DAP v1.0 debug probe for SWD, with a UART bridge on GP0 and GP1. It works with OpenOCD and other CMSIS-DAP tools, and no port name is needed.

```sh
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg
```

## Wiring Reference

<div align="center">

![PROBINIX pinout](images/PROBINIX-Pinout.png?raw=true "PROBINIX pinout")

</div>

### Pin summary

| Interface | GP0 | GP1 | GP2 | GP3 | GP4 |
|---|---|---|---|---|---|
| UART | TX | RX | | | |
| JTAG | TCK | TMS | TDI | TDO | |
| SWD | | | SWCLK | SWDIO | |
| SPI | | CS | CLK | MOSI | MISO |
| I2C (default) | SDA | SCL | | | |

Always connect **GND** to the target's GND. Connect PROBINIX's TX to the target's RX, and PROBINIX's RX to the target's TX.

### Per mode

<table>
<tr>
<td valign="top">

#### USB-to-serial

| PROBINIX | Target |
|---|---|
| GP0 (TX) | RX |
| GP1 (RX) | TX |
| GND | GND |

</td>
<td valign="top">

#### SPI flash

Used by `F` and `f`.

| PROBINIX | SPI flash |
|---|---|
| GP1 | CS |
| GP2 | CLK |
| GP3 | MOSI / DI |
| GP4 | MISO / DO |
| GND | GND |

</td>
</tr>
<tr>
<td valign="top">

#### OpenOCD

| PROBINIX | JTAG | SWD |
|---|---|---|
| GP0 | TCK | |
| GP1 | TMS | |
| GP2 | TDI | SWCLK |
| GP3 | TDO | SWDIO |
| GND | GND | GND |

</td>
<td valign="top">

#### CMSIS-DAP

| PROBINIX | Target |
|---|---|
| GP0 (TX) | UART RX |
| GP1 (RX) | UART TX |
| GP2 | SWCLK |
| GP3 | SWDIO |
| GND | GND |

</td>
</tr>
</table>

If the target has no power of its own, you can power it from PROBINIX's **3V3 OUT** pin.

## Hardware Boot Modes

PROBINIX can start directly in a hardware mode, without the menu. This is useful when you use the same setup often.

| Connect to GND | Mode at startup |
|---|---|
| GP26 | USB-to-serial |
| GP27 | OpenOCD JTAG/SWD |
| GP28 | CMSIS-DAP v1.0 |

Connect one pin to GND, then reset or reconnect the board. Only one boot mode can be active at a time. Disconnect the pin to return to the menu.

## Waveshare RP2040-PiZero

The [Waveshare RP2040-PiZero](https://www.waveshare.com/wiki/RP2040-PiZero) has a Raspberry Pi style 40-pin header, 16 MB of flash, a micro SD slot, an HDMI port and a second PIO-USB port. It uses its own firmware file, `PROBINIX-v1.0-RP2040-PiZero.uf2`.

### Flashing

1. Connect the USB-C port labeled **USB**. The port labeled **PIO-USB** does not work for flashing or the console.
2. Hold **BOOT**, press and release **RUN**, then release **BOOT**. A drive named `RPI-RP2` appears.
3. Copy `PROBINIX-v1.0-RP2040-PiZero.uf2` onto the drive.

### Header pins

PROBINIX uses the same GPIO numbers on every board, but on the PiZero they sit at different header positions. The console shows the header pin next to each GPIO in the menu and in every wiring list.

| GPIO | Header pin | PROBINIX use | Onboard connection |
|---|:---:|---|---|
| GP0 | 27 | UART TX, JTAG TCK, I2C SDA | |
| GP1 | 28 | UART RX, JTAG TMS, SPI CS, I2C SCL | |
| GP2 | 3 | JTAG TDI, SWD SWCLK, SPI CLK | HDMI control line with 2.2 kΩ pull-up |
| GP3 | 5 | JTAG TDO, SWD SWDIO, SPI MOSI | HDMI control line with 2.2 kΩ pull-up |
| GP4 | 8 | SPI MISO | |
| GP5 | 10 | Scan channel | |
| GP6 | 31 | Scan channel | PIO-USB D+ with 2.2 kΩ pull-up |
| GP7 | 26 | Scan channel | PIO-USB D- |
| GP8 | 24 | Scan channel | |
| GP9 | 32 | Scan channel | |
| GP10 | 23 | Scan channel | |
| GP11 | 19 | Scan channel | |
| GP12 | 21 | Scan channel | |
| GP13 | 33 | Scan channel | |
| GP14 | 7 | Scan channel | |
| GP15 | 29 | Scan channel | |

Power pins: **3V3** on pins 1 and 17, **5V** on pins 2 and 4, and **GND** on pins 6, 9, 14, 20, 25, 30, 34 and 39.

### Boot modes

GP28 is not on the PiZero header, so the CMSIS-DAP boot pin moves to GP17.

| Connect to GND | Header pin | Mode at startup |
|---|:---:|---|
| GP26 | 37 | USB-to-serial |
| GP27 | 13 | OpenOCD JTAG/SWD |
| GP17 | 11 | CMSIS-DAP v1.0 |

### Differences from the Pico

- **No status light:** the PiZero has only a power LED, so PROBINIX does not use an activity LED.
- **Onboard pull-ups:** GP2, GP3 and GP6 have 2.2 kΩ pull-up resistors, so the pin monitor shows them as `High` when nothing is connected. JTAG, SWD and other tools work normally.
- **Shared pins:** GP18 to GP21 connect to the micro SD slot and GP16 and GP22 to GP29 connect to the HDMI port. Leave the SD slot empty and HDMI unplugged while using PROBINIX.

## Troubleshooting

### General

| Problem | Solution |
|---|---|
| No `RPI-RP2` drive appears | Hold BOOTSEL before plugging in, and use a USB cable that carries data, not only power. |
| Every key appears twice | Turn off local echo in your terminal. |
| Port missing on Linux after a mode starts | The port name can change after PROBINIX reconnects. Run `ls /dev/ttyACM*`. If ModemManager is using the port, run `sudo systemctl stop ModemManager`. |
| `Permission denied` on Linux | Run `sudo usermod -aG dialout $USER`, then log out and back in. |
| A tool can't open the port | Close your terminal program first. Only one program can use the port at a time. |
| Windows shows an old device name | Remove PROBINIX in Device Manager, then plug it in again. |

### Scans and modes

| Problem | Solution |
|---|---|
| JTAG or SWD scan finds nothing | Check GND, make sure the target is powered, and try toggling pin pulsing (`p`). |
| Blank screen in USB-to-serial mode | The target's TX must go to GP1. Check the baud rate, then reset the target, because many targets only print while booting. |
| Garbled serial text | The baud rate is wrong. Run `u` to detect it. |
| Unsure whether PROBINIX or the target is the problem | Connect GP0 to GP1 with nothing else attached, start `U`, and type. If you see your typing, the PROBINIX bridge works. |
| UART finder finds no RX | The target may ignore input or have its console disabled. Wait until it has finished booting. |
| Flashrom reads fail | Check wiring with `f` first, use shorter wires, and try `spispeed=1M`. For chips still on a board, hold the board's main processor in reset. |
| EEPROM tool says no reply | Check SDA, SCL and GND, the chip type, and the A2-A0 address value. Run `i` to see which address the chip answers on. |

## Safety Notes

> [!WARNING]
> - PROBINIX uses **3.3 V logic**. Use level shifters for targets at other voltages.
> - Always connect **GND** between PROBINIX and the target first.
> - Keep GP0 to GP15 **away from GND** while scanning.
> - Power the target from **3V3 OUT** only if it has no other power source.
> - JTAG scans, SWD scans, RTCK detection, the UART RX search, I2C scans, SPI tools and manual pin control drive pins. Connect only to the test points you intend to test.

> [!IMPORTANT]
> Use PROBINIX only on hardware you own or are authorized to test.

## Building from Source

### Using Docker

```sh
cd PROBINIX
docker build -t probinix-builder .
docker create --name probinix-build probinix-builder
docker cp probinix-build:/project/src/build_rp2040/PROBINIX.uf2 ./PROBINIX-RP2040.uf2
docker rm probinix-build
```

### Using the Pico SDK

Requirements: [Pico SDK 2.1.0](https://github.com/raspberrypi/pico-sdk), CMake 3.13 or later, and the Arm GNU Toolchain.

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S src -B build -DPICO_BOARD=pico
cmake --build build
```

The firmware is written to `build/PROBINIX.uf2`. The same file runs on the Pico and Pico W.

To build for the Waveshare RP2040-PiZero, add the `PROBINIX_PIZERO` option:

```sh
cmake -S src -B build-pizero -DPICO_BOARD=pico -DPROBINIX_PIZERO=ON
cmake --build build-pizero
```

## Project Structure

```text
PROBINIX/
├── firmware/            Prebuilt UF2 firmware
├── images/              README images
├── src/
│   ├── PROBINIX.c       Console, menu, JTAG and SWD scanning
│   ├── usbDescriptors.c USB device descriptors
│   └── modules/
│       ├── recon/           Pin monitor, UART finder, I2C scanner, SPI flash ID and EEPROM dump
│       ├── usb2serial/      USB-to-serial bridge
│       ├── flashProgrammer/ Flashrom serprog support
│       ├── openocd/         OpenOCD Bus Pirate support
│       └── cmsis-dap/       CMSIS-DAP debugger support
├── Dockerfile           Reproducible firmware build
└── CHANGELOG.md         Release history
```

## Acknowledgments

PROBINIX builds on the work of these projects and references.

**JTAG and SWD**
- [JTAGulator](https://github.com/grandideastudio/jtagulator) by Joe Grand
- [SWD for ARM: Alternative to JTAG](https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/)
- [PicoReg](https://github.com/jbentham/picoreg)
- [SWDscan](https://github.com/szymonh/SWDscan)
- [Yusufss4's SWD notes](https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
- [Arm Debug Interface Architecture Specification](https://www.arm.com/architecture-debug-interface)

**Flashrom**
- [Serprog protocol](https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html)
- [pico-serprog](https://github.com/stacksmashing/pico-serprog)

**OpenOCD**
- [Bus Pirate raw bitbang mode](http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/)
- [Bus Pirate binary raw wire mode](http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/)
- [JTAGulator PropOCD](https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin)
- [Bus Pirate binary I/O firmware](https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c)

**USB-to-serial**
- [pico-uart-bridge by xxxajk](https://github.com/xxxajk/pico-uart-bridge)
- [pico-uart-bridge by Noltari](https://github.com/Noltari/pico-uart-bridge)

**CMSIS-DAP**
- [DapperMime](https://github.com/majbthrd/DapperMime)
- [Raspberry Pi Debug Probe](https://github.com/raspberrypi/debugprobe)

## License

| Component | License |
|---|---|
| PROBINIX project code | MIT, unless otherwise stated |
| TinyUSB | MIT |
| Arm CMSIS_5 | Apache 2.0 |

PROBINIX includes code by Aodrulez, used under the MIT License. See [LICENSE](https://github.com/cybermax560-exe/ProbeX/blob/main/LICENSE).
