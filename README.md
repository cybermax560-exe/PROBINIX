<div align="center">

# PROBINIX

**A JTAGulator-inspired hardware hacking multi-tool for RP2040 development boards**

![Version](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-RP2040-red)
![License](https://img.shields.io/badge/license-MIT-green)

</div>

---

PROBINIX turns an inexpensive RP2040 board, such as the Raspberry Pi Pico, into a versatile tool for hardware security research and embedded debugging. It finds JTAG and SWD debug ports, identifies UART, I2C and SPI flash interfaces, and works as a USB-to-serial adapter, flash programmer, OpenOCD adapter or CMSIS-DAP debugger, all through a simple serial terminal menu.

<div align="center">

![PROBINIX interface](images/PROBINIX-Interface.png?raw=true "PROBINIX v1.0 interface")

</div>

## Table of Contents

- [Features](#features)
- [Supported Hardware](#supported-hardware)
- [Quick Start](#quick-start)
- [Command Reference](#command-reference)
- [Wiring Reference](#wiring-reference)
- [Hardware Boot Modes](#hardware-boot-modes)
- [Building from Source](#building-from-source)
- [Safety Notes](#safety-notes)
- [Project Structure](#project-structure)
- [Acknowledgments](#acknowledgments)
- [License](#license)

## Features

### Debug interface discovery

- **JTAG pinout scan:** finds TDI, TDO, TCK, TMS and TRST across up to 16 channels, then reads and identifies device IDs.
- **SWD pinout scan:** finds SWDIO and SWCLK and reads the target's debug port ID.

### Target reconnaissance

- **Pin monitor:** shows whether each channel is high, low, floating or active, with edge count, approximate frequency and duty cycle.
- **UART finder:** listens passively while the target boots and reports likely TX pins and standard baud rates from 1200 to 460800.
- **I2C scanner:** lists responding device addresses on the SDA and SCL pins you choose.
- **SPI flash identification:** reads the JEDEC ID and reports the manufacturer and typical capacity.

### Hardware modes

- **USB-to-serial adapter:** a standard UART bridge for target consoles.
- **Flash programmer:** reads and writes SPI flash chips with [Flashrom](https://www.flashrom.org/) using the serprog protocol.
- **OpenOCD adapter:** JTAG and SWD debugging through the Bus Pirate protocol.
- **CMSIS-DAP v1.0 debugger:** SWD debugging with a UART bridge.

## Supported Hardware

| Board | Status |
|---|---|
| Raspberry Pi Pico and other RP2040 boards | Supported |
| Waveshare RP2040-Zero | Supported, see the pinout below |
| RP2350 boards, such as the Raspberry Pi Pico 2 | Experimental, no firmware in v1.0 |

All channel inputs and outputs use **3.3 V logic**.

## Quick Start

### 1. Flash the firmware

1. Download `PROBINIX-v1.0-RP2040.uf2` from the [`firmware`](firmware) folder or the [Releases](../../releases) page.
2. Hold the **BOOTSEL** button and connect the board to your computer over USB.
3. Release the button when the `RPI-RP2` drive appears.
4. Copy the `.uf2` file onto the drive. The board restarts automatically.

### 2. Connect to PROBINIX

1. Open a serial terminal such as [PuTTY](https://www.putty.org/), [Tera Term](https://teratermproject.github.io/) or `minicom`.
2. Select the COM port assigned to PROBINIX. The baud rate setting does not matter.
3. Disable local echo in your terminal.
4. Press any key to show the menu.

### 3. Connect to a target

1. Connect **GND** on the board to **GND** on the target.
2. Connect channels **GPIO 0 to GPIO 15** to the target's test points.
3. Choose a command from the menu.

> [!TIP]
> Scans check channels in order, so connecting test points to GPIO 0, 1, 2 and so on gives the fastest results.

## Command Reference

| Key | Command | Description |
|:---:|---|---|
| `h` | Help | Show the command menu |
| `v` | Version | Show the firmware version |
| `p` | Pin pulsing | Toggle pin pulsing for JTAG and SWD scans. Default: off |
| `j` | JTAG scan | Find a JTAG pinout using 4 to 16 channels |
| `s` | SWD scan | Find an SWD pinout |
| `m` | Pin monitor | Report pin state and activity for 1 second |
| `u` | UART finder | Listen for 5 seconds while the target boots |
| `i` | I2C scan | Probe addresses `0x08` to `0x77` at about 50 kHz |
| `f` | SPI flash ID | Read the JEDEC ID of an SPI flash chip |
| `U` | USB-to-serial | Start the UART bridge |
| `F` | Flashrom | Start serprog flash programmer mode |
| `O` | OpenOCD | Start JTAG or SWD debugger mode |
| `C` | CMSIS-DAP | Start CMSIS-DAP v1.0 debugger mode |

Hardware modes stay active until you unplug or reset the board.

## Wiring Reference

<div align="center">

![PROBINIX pinout](images/PROBINIX-Pinout.png?raw=true "PROBINIX pinout")

</div>

<table>
<tr>
<td valign="top">

**USB-to-serial (`U`)**

| PROBINIX | Target |
|---|---|
| GPIO 0 (TX) | RX |
| GPIO 1 (RX) | TX |
| GND | GND |

</td>
<td valign="top">

**Flashrom and SPI flash ID (`F`, `f`)**

| PROBINIX | SPI flash |
|---|---|
| GPIO 1 | CS |
| GPIO 2 | CLK |
| GPIO 3 | MOSI / DI |
| GPIO 4 | MISO / DO |
| GND | GND |

</td>
</tr>
<tr>
<td valign="top">

**OpenOCD (`O`)**

| PROBINIX | JTAG | SWD |
|---|---|---|
| GPIO 0 | TCK | |
| GPIO 1 | TMS | |
| GPIO 2 | TDI | SWCLK |
| GPIO 3 | TDO | SWDIO |
| GND | GND | GND |

</td>
<td valign="top">

**CMSIS-DAP (`C`)**

| PROBINIX | Target |
|---|---|
| GPIO 0 (TX) | UART RX |
| GPIO 1 (RX) | UART TX |
| GPIO 2 | SWCLK |
| GPIO 3 | SWDIO |
| GND | GND |

</td>
</tr>
</table>

The I2C scanner asks for its SDA and SCL pins. The defaults are GPIO 0 for SDA and GPIO 1 for SCL.

### Example OpenOCD commands

Replace `COM4` with your PROBINIX port, such as `/dev/ttyACM0` on Linux.

```sh
# JTAG through Bus Pirate mode
openocd -f interface/buspirate.cfg -c "buspirate port COM4; transport select jtag;" -f target/esp32.cfg

# SWD through Bus Pirate mode
openocd -f interface/buspirate.cfg -c "buspirate port COM4; transport select swd;" -f target/stm32f1x.cfg

# CMSIS-DAP mode
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg
```

### Example Flashrom commands

```sh
flashrom -p serprog:dev=COM4:115200,spispeed=12M -r backup.bin
flashrom -p serprog:dev=COM4:115200,spispeed=12M -w firmware.bin
```

## Hardware Boot Modes

PROBINIX can start directly in a hardware mode without using the menu. Connect one boot pin to GND, then reset or reconnect the board.

| Boot pin connected to GND | Mode at startup |
|---|---|
| GPIO 26 | USB-to-serial |
| GPIO 27 | OpenOCD JTAG/SWD debugger |
| GPIO 28 | CMSIS-DAP v1.0 debugger |

Only one boot mode can be active at a time. Disconnect the pin from GND to return to the normal menu.

## Building from Source

### Using Docker (recommended)

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

The firmware is written to `build/PROBINIX.uf2`.

## Safety Notes

> [!WARNING]
> - PROBINIX uses **3.3 V logic**. Use level shifters for targets at other voltages.
> - Always connect **GND** between PROBINIX and the target first.
> - Keep GPIO 0 to GPIO 15 **away from GND** while scanning.
> - Power the target from **3V3 Out** only if it has no other power source.
> - For in-circuit SPI flash, hold the target's main processor in reset so only PROBINIX drives the bus.
> - The I2C scanner reads one byte from each responding device. Some devices change state when read.

> [!IMPORTANT]
> Use PROBINIX only on hardware you own or are authorized to test.

## Project Structure

```text
PROBINIX/
├── firmware/            Prebuilt UF2 firmware
├── images/              README images
├── src/
│   ├── PROBINIX.c       Main menu, JTAG and SWD scanning
│   ├── usbDescriptors.c USB device descriptors
│   └── modules/
│       ├── recon/           Pin monitor, UART, I2C and SPI flash tools
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
