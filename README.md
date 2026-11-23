# [ PROBINIX ] 

JTAGulator-inspired multi-tool for hardware hackers, built for RP2040 microcontroller based development boards.

## Features
- Detects JTAG & SWD debug pinout (JTAGulator function)  
- Functions as a USB-to-Serial adapter  
- Reads & writes flash ICs with Flashrom  
- Supports OpenOCD with JTAG & SWD modes (BusPirate protocol)  
- Acts as a CMSIS-DAP adapter (supports UART & SWD)  
- Monitors pin states and signal activity (`m`)  
- Finds UART TX pins and baud rates passively (`u`)  
- Scans I2C buses for devices (`i`, uses read transactions)  
- Identifies SPI flash chips by JEDEC ID (`f`, uses Flashrom mode wiring)  

## Pinout
![](images/PROBINIX-Pinout.png?raw=true "PROBINIX Pinout")

## Installation
- Get the PROBINIX firmware file ("PROBINIX-vX.X.X-RP2040.uf2")
- Press & hold 'BOOTSEL' button on a RP2040 microcontroller based development board, connect it to a computer via USB cable & then release the button
- Copy "PROBINIX-vX.X.X.uf2" file onto the newly detected flash drive (RPI-RP2*) on your computer


## Usage
- Connect the RP2040 microcontroller based development board running PROBINIX to your computer using USB cable
- Connect the development board's GPIO pins (GPIO0-GPIO15 so 16 channels in all) to your target's test-points
- Connect the development board's "GND" pin to target's "GND"
- Open a terminal emulator program of your choice that supports "Serial" communication (Ex. Teraterm, Putty, Minicom)
- Select "Serial" communication & connect to the development board's newly assigned COM port
- PROBINIX supports auto-baudrate detection so you should not have to perform any additional settings
- Press any key in the terminal emulator program to start using PROBINIX
- UI provides detailed guidance for all commands & hardware modes

> **_Hardware boot modes:_** If you want PROBINIX to boot straight into a hardware mode, connect the relevant hardware boot mode selection GPIO to GND & reset or reconnect PROBINIX to your computer. As long as the GPIO is connected to GND, PROBINIX will continue to boot into the selected hardware mode when reset or reconnected. Only one boot mode can be active at a time. 

> **_NOTE 1:_** Most RP2040 microcontroller based development board's GPIO pins function at 3.3v. For connecting to devices running other voltage levels, use of external level shifter(s) will be required.

> **_NOTE 2:_** Since the algorithm verifies channels in order (from 0 to 15), connect the channels in sequence (from 0 to 15) to your target's testpads/test points for the quickest execution time when using JTAGulator function.  

## Local build instructions
> **Recommended to use Docker**
- Open a command-prompt or terminal
- Change to the desired directory & then execute the following commands:
```sh
cd PROBINIX
docker build -t pico-builder-image .
docker create --name pico-builder-container pico-builder-image
docker cp pico-builder-container:/project/src/build_rp2040/PROBINIX.uf2 ./PROBINIX-RP2040.uf2
docker cp pico-builder-container:/project/src/build_rp2350/PROBINIX.uf2 ./PROBINIX-RP2350.uf2
```
> _NOTE :_ RP2350 builds are only for testing at the moment.

## References & Acknowledgments
- **JTAGulator Features:**
  - [JTAGulator Project](https://github.com/grandideastudio/jtagulator)
  - [SWD for ARM: Alternative to JTAG](https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/)
  - [PicoReg: A JTAGulator Tool for RP2040](https://github.com/jbentham/picoreg)
  - [SWDscan by Szymon H](https://github.com/szymonh/SWDscan)
  - [Yusufss4's Work on SWD](https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
  - [ARM Debug Interface Architecture Specification](https://www.arm.com/architecture-debug-interface)
        
- **Flashrom Support:**
  - [Flashrom Supported Hardware](https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html)
  - [Pico-serprog: Flash Programming via RP2040](https://github.com/stacksmashing/pico-serprog)

- **OpenOCD Support:**
  - [BusPirate Raw Bitbang Mode](http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/)
  - [Binary Raw Wire Mode with BusPirate](http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/)
  - [JTAGulator for OpenOCD](https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin)
  - [BusPirate Firmware for Binary I/O](https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c)

- **USB-to-Serial Support:**
  - [Pico UART Bridge](https://github.com/xxxajk/pico-uart-bridge)
  - [Noltari's Pico UART Bridge](https://github.com/Noltari/pico-uart-bridge)

- **CMSIS-DAP Support:**
  - [DapperMime: CMSIS-DAP Debugging Tool](https://github.com/majbthrd/DapperMime)
  - [Raspberry Pi Debug Probe](https://github.com/raspberrypi/debugprobe)

## License
- TinyUSB: MIT License  
- ARM CMSIS_5: Apache 2.0 License  
- Project Code: MIT License (unless otherwise stated)

PROBINIX includes code by Aodrulez, used under the MIT License. See [LICENSE](https://github.com/cybermax560-exe/ProbeX/blob/main/LICENSE).
