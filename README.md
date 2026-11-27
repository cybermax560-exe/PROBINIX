<div align="center">

# PROBINIX

**A JTAGulator-inspired hardware hacking multi-tool for RP2040 boards**

![Version](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-RP2040-red)
![Boards](https://img.shields.io/badge/boards-Pico%20%7C%20Pico%20W-brightgreen)
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
| Find UART TX, RX and baud rate | Scan I2C buses | Flashrom SPI programmer |
| Identify chips by IDCODE | Identify SPI flash chips | OpenOCD and CMSIS-DAP debugger |

## Table of Contents

- [Supported Boards](#supported-boards)
- [Quick Start](#quick-start)
- [Using the Console](#using-the-console)
- [Command Reference](#command-reference)
- [Tool Guide](#tool-guide)
- [Wiring Reference](#wiring-reference)
- [Hardware Boot Modes](#hardware-boot-modes)
- [Troubleshooting](#troubleshooting)
- [Safety Notes](#safety-notes)
- [Building from Source](#building-from-source)
- [Project Structure](#project-structure)
- [Acknowledgments](#acknowledgments)
- [License](#license)

## Supported Boards

PROBINIX runs on boards built around the **RP2040** microcontroller. One firmware file works on all of the boards below.

| Board | Status | Notes |
|---|---|---|
| **Raspberry Pi Pico** | ✅ Supported | Reference board for PROBINIX |
| **Raspberry Pi Pico H** | ✅ Supported | Same as Pico, with pre-soldered headers |
| **Raspberry Pi Pico W** | ✅ Supported | Wi-Fi and Bluetooth are not used. The onboard LED is not used as a status light. |
| **Raspberry Pi Pico WH** | ✅ Supported | Same as Pico W, with pre-soldered headers |
| **Waveshare RP2040-Zero** | ✅ Supported | Compact board. See the [pinout image](#wiring-reference). |
| Other RP2040 boards | ⚠️ Should work | The board must expose GP0 to GP15. GP26 to GP28 are needed only for boot modes. |
| RP2350 boards, such as Pico 2 | 🧪 Experimental | No firmware file in v1.0 |

> [!NOTE]
> On the Pico and Pico H, the onboard LED stays on while PROBINIX is ready and flickers during JTAG scans. The Pico W connects its LED through the wireless chip, so the LED stays off. Everything else works the same.

## Quick Start

### 1. Flash the firmware

1. Download `PROBINIX-v1.0-RP2040.uf2` from the [`firmware`](firmware) folder or the [Releases](../../releases) page.
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
| `p` | Pin pulsing | Changes how idle channels are pulled during JTAG and SWD scans |
| `m` | Pin monitor | Shows each channel's state, edges, frequency and duty cycle |
| `u` | UART finder | Finds the target's TX pin and baud rate, then optionally its RX pin |
| `i` | I2C scanner | Lists the addresses of devices on an I2C bus |
| `f` | SPI flash ID | Reads the manufacturer and size of an SPI flash chip |
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

## Safety Notes

> [!WARNING]
> - PROBINIX uses **3.3 V logic**. Use level shifters for targets at other voltages.
> - Always connect **GND** between PROBINIX and the target first.
> - Keep GP0 to GP15 **away from GND** while scanning.
> - Power the target from **3V3 OUT** only if it has no other power source.
> - JTAG scans, SWD scans, the UART RX search, I2C scans and SPI tools drive pins. Connect only to the test points you intend to test.

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

## Project Structure

```text
PROBINIX/
├── firmware/            Prebuilt UF2 firmware
├── images/              README images
├── src/
│   ├── PROBINIX.c       Console, menu, JTAG and SWD scanning
│   ├── usbDescriptors.c USB device descriptors
│   └── modules/
│       ├── recon/           Pin monitor, UART finder, I2C scanner and SPI flash ID
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
