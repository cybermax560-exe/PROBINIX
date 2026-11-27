/*
    Target reconnaissance tools for PROBINIX.

    Pin monitor and UART scan only listen. The I2C scan and SPI flash ID drive
    the pins the user selects, but issue read-only bus transactions.
*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"
#include "recon.h"

#define EOL             "\r\n"
#define BOLD            "\033[1m"
#define OFF             "\033[0m"
#define CLR_HEAD        "\033[1;36m"
#define CLR_DIM         "\033[90m"
#define CLR_OK          "\033[1;32m"
#define CHANNELS        16
#define MAX_WIDTHS      128
#define SYSTICK_MASK    0x00FFFFFFu

#define MONITOR_MS      1000
#define UART_SCAN_MS    5000
#define UART_MIN_PULSES 16
#define UART_RX_QUIET_MS 150
#define UART_RX_REPLY_MS 400
#define UART_RX_MIN_EDGES 4

#define I2C_HALF_US     10      // About 50 kHz, safe with internal pull-ups
#define I2C_STRETCH_US  2000
#define I2C_FIRST_ADDR  0x08
#define I2C_LAST_ADDR   0x77

#define SPI_CS          1       // Same wiring as Flashrom mode
#define SPI_CLK         2
#define SPI_MOSI        3
#define SPI_MISO        4
#define SPI_HALF_US     1
#define SPI_ID_LEN      12

const char *jep106_table_manufacturer(unsigned int bank, unsigned int id);

typedef struct {
    uint32_t edges;
    uint32_t lastEdge;
    uint32_t highCycles;
    uint32_t lowCycles;
    uint32_t widthCount;
    uint32_t widths[MAX_WIDTHS];
} channelCapture;

static channelCapture capture[CHANNELS];

static const uint32_t standardBauds[] = {
    460800, 230400, 115200, 76800, 57600, 38400, 19200, 9600, 4800, 2400, 1200
};

static void reconTitle(const char *title)
{
    printf("  " CLR_HEAD "%s" OFF " " CLR_DIM, title);
    for (int i = (int)strlen(title) + 1; i < 76; i++)
    {
        putchar('-');
    }
    printf(OFF "%s%s", EOL, EOL);
}

//------------------------------------------ Input -------------------------------------------------

// Read a decimal number. Enter alone selects defaultValue when it is >= 0.
static int readNumber(int minValue, int maxValue, int defaultValue)
{
    static bool lastWasCR = false;

    while (true)
    {
        char digits[4];
        int length = 0;

        while (true)
        {
            int c = getc(stdin);
            if (c == '\n' && lastWasCR)
            {
                lastWasCR = false;      // Second half of a CRLF line ending
                continue;
            }
            lastWasCR = (c == '\r');
            if (c == '\r' || c == '\n')
            {
                break;
            }
            if ((c == '\b' || c == 127) && length > 0)
            {
                length--;
                printf("\b \b");
            }
            else if (c >= '0' && c <= '9' && length < 3)
            {
                digits[length++] = (char)c;
                putchar(c);
            }
            fflush(stdout);
        }
        printf("%s", EOL);

        if (length == 0 && defaultValue >= 0)
        {
            return defaultValue;
        }
        if (length > 0)
        {
            int value = 0;
            for (int i = 0; i < length; i++)
            {
                value = value * 10 + (digits[i] - '0');
            }
            if (value >= minValue && value <= maxValue)
            {
                return value;
            }
        }
        printf("     Enter a value from %d to %d: ", minValue, maxValue);
        fflush(stdout);
    }
}

static int readChannelCount(void)
{
    printf("     Channels to check, starting at GPIO 0 (1-%d) [%d]: ", CHANNELS, CHANNELS);
    fflush(stdout);
    return readNumber(1, CHANNELS, CHANNELS);
}

//------------------------------------------ Pin setup ---------------------------------------------

static void listenOnChannels(int count)
{
    for (int pin = 0; pin < count; pin++)
    {
        gpio_init(pin);
        gpio_set_pulls(pin, false, true);   // Keeps unconnected pins from toggling
    }
    sleep_us(50);
}

static void releasePin(uint pin)
{
    gpio_init(pin);
    gpio_disable_pulls(pin);
}

static void releaseChannels(int count)
{
    for (int pin = 0; pin < count; pin++)
    {
        releasePin(pin);
    }
}

//------------------------------------------ Edge capture ------------------------------------------

// Runs from RAM so flash cache misses do not disturb pulse timing.
static void __not_in_flash_func(captureEdges)(uint32_t mask, uint32_t totalCycles)
{
    uint32_t savedCsr = systick_hw->csr;
    uint32_t savedRvr = systick_hw->rvr;

    memset(capture, 0, sizeof(capture));
    systick_hw->csr = 0;
    systick_hw->rvr = SYSTICK_MASK;
    systick_hw->cvr = 0;
    systick_hw->csr = 5;                    // Enabled, counting CPU cycles

    uint32_t previous = gpio_get_all() & mask;
    uint32_t lastTick = systick_hw->cvr;
    uint32_t elapsed = 0;

    while (elapsed < totalCycles)
    {
        uint32_t tick = systick_hw->cvr;
        elapsed += (lastTick - tick) & SYSTICK_MASK;
        lastTick = tick;

        uint32_t current = gpio_get_all() & mask;
        uint32_t changed = current ^ previous;
        previous = current;

        while (changed)
        {
            uint pin = __builtin_ctz(changed);
            changed &= changed - 1;

            channelCapture *c = &capture[pin];
            uint32_t width = elapsed - c->lastEdge;

            if (current & (1u << pin))
            {
                c->lowCycles += width;      // Rising edge ends a low period
            }
            else
            {
                c->highCycles += width;
            }
            if (c->edges > 0 && c->widthCount < MAX_WIDTHS)
            {
                c->widths[c->widthCount++] = width;
            }
            c->lastEdge = elapsed;
            c->edges++;
        }
    }

    for (uint pin = 0; pin < CHANNELS; pin++)
    {
        channelCapture *c = &capture[pin];
        if (c->edges > 0)
        {
            if (previous & (1u << pin))
            {
                c->highCycles += elapsed - c->lastEdge;
            }
            else
            {
                c->lowCycles += elapsed - c->lastEdge;
            }
        }
    }

    systick_hw->csr = 0;
    systick_hw->rvr = savedRvr;
    systick_hw->cvr = 0;
    systick_hw->csr = savedCsr;
}

//------------------------------------------ Pin monitor -------------------------------------------

static const char *probePinState(uint pin)
{
    gpio_set_pulls(pin, true, false);
    sleep_us(30);
    bool withPullUp = gpio_get(pin);
    gpio_set_pulls(pin, false, true);
    sleep_us(30);
    bool withPullDown = gpio_get(pin);

    if (withPullUp && !withPullDown)
    {
        return "Floating";
    }
    if (withPullUp && withPullDown)
    {
        return "High";
    }
    if (!withPullUp && !withPullDown)
    {
        return "Low";
    }
    return "Changing";
}

void reconPinMonitor(void)
{
    const char *states[CHANNELS];

    reconTitle("PIN MONITOR");
    int count = readChannelCount();

    listenOnChannels(count);
    for (int pin = 0; pin < count; pin++)
    {
        states[pin] = probePinState(pin);
    }

    printf("%s     Listening for %d ms...%s%s", EOL, MONITOR_MS, EOL, EOL);
    fflush(stdout);
    captureEdges((1u << count) - 1, (clock_get_hz(clk_sys) / 1000) * MONITOR_MS);
    releaseChannels(count);

    printf("     +------+----------+----------+------------+-------+%s", EOL);
    printf("     | GPIO | State    | Edges    | Approx Hz  | High  |%s", EOL);
    printf("     +------+----------+----------+------------+-------+%s", EOL);
    for (int pin = 0; pin < count; pin++)
    {
        const channelCapture *c = &capture[pin];
        if (c->edges > 1)
        {
            uint64_t total = (uint64_t)c->highCycles + c->lowCycles;
            uint32_t duty = total ? (uint32_t)(((uint64_t)c->highCycles * 100) / total) : 0;
            uint32_t hz = (c->edges / 2) * (1000 / MONITOR_MS);
            printf("     | %4d | %-8s | %8lu | %10lu | %4lu%% |%s", pin, "Active",
                   (unsigned long)c->edges, (unsigned long)hz, (unsigned long)duty, EOL);
        }
        else
        {
            printf("     | %4d | %-8s | %8lu | %10s | %5s |%s", pin, states[pin],
                   (unsigned long)c->edges, "-", "-", EOL);
        }
    }
    printf("     +------+----------+----------+------------+-------+%s%s", EOL, EOL);
    printf("     Very fast signals can be undercounted.%s%s", EOL, EOL);
}

//------------------------------------------ UART scan ---------------------------------------------

// A UART line idles high and its pulse widths are whole multiples of one bit.
static uint32_t detectBaud(const channelCapture *c, uint32_t clockHz)
{
    if (c->widthCount < UART_MIN_PULSES || c->highCycles <= c->lowCycles)
    {
        return 0;
    }

    for (size_t b = 0; b < sizeof(standardBauds) / sizeof(standardBauds[0]); b++)
    {
        uint32_t bit = clockHz / standardBauds[b];
        uint32_t singleBits = 0, considered = 0, fitting = 0;
        bool hasLongerPulse = false;

        for (uint32_t i = 0; i < c->widthCount; i++)
        {
            uint32_t width = c->widths[i];
            if (width > bit * 10 + bit / 2)
            {
                hasLongerPulse = true;      // Idle gap between transmissions
                continue;
            }

            uint32_t multiple = (width + bit / 2) / bit;
            if (multiple == 0)
            {
                multiple = 1;
            }
            uint32_t ideal = multiple * bit;
            uint32_t error = (width > ideal) ? width - ideal : ideal - width;

            considered++;
            if (error <= bit / 4)
            {
                fitting++;
                if (multiple == 1)
                {
                    singleBits++;
                }
                else
                {
                    hasLongerPulse = true;
                }
            }
        }

        uint32_t minimumSingles = (considered / 16 > 3) ? considered / 16 : 3;
        if (considered >= UART_MIN_PULSES && singleBits >= minimumSingles &&
            hasLongerPulse && fitting * 100 >= considered * 85)
        {
            return standardBauds[b];
        }
    }
    return 0;
}

// Send text on any pin as 8N1 UART, timed by CPU cycles. Runs from RAM with interrupts off.
static void __not_in_flash_func(uartBitBangSend)(uint pin, const char *text, uint32_t cyclesPerBit)
{
    uint32_t savedCsr = systick_hw->csr;
    uint32_t savedRvr = systick_hw->rvr;

    systick_hw->csr = 0;
    systick_hw->rvr = SYSTICK_MASK;
    systick_hw->cvr = 0;
    systick_hw->csr = 5;

    uint32_t interrupts = save_and_disable_interrupts();
    uint32_t lastTick = systick_hw->cvr;
    uint32_t elapsed = 0;
    uint32_t nextEdge = 0;

    for (const char *p = text; *p; p++)
    {
        // Bit 0 = start (low), bits 1-8 = data LSB first, bit 9 = stop (high)
        uint32_t frame = ((uint32_t)(uint8_t)*p << 1) | (1u << 9);
        for (int bit = 0; bit < 10; bit++)
        {
            gpio_put(pin, (frame >> bit) & 1);
            nextEdge += cyclesPerBit;
            while (elapsed < nextEdge)
            {
                uint32_t tick = systick_hw->cvr;
                elapsed += (lastTick - tick) & SYSTICK_MASK;
                lastTick = tick;
            }
        }
    }

    restore_interrupts(interrupts);
    systick_hw->csr = 0;
    systick_hw->rvr = savedRvr;
    systick_hw->cvr = 0;
    systick_hw->csr = savedCsr;
}

static uint32_t countEdgesOn(uint pin, uint32_t milliseconds)
{
    captureEdges(1u << pin, (clock_get_hz(clk_sys) / 1000) * milliseconds);
    return capture[pin].edges;
}

// Drive each candidate pin, send Enter, and watch the target's TX pin for a reply.
static int findUartRx(int count, uint txPin, uint32_t baud, uint32_t skipMask)
{
    uint32_t clockHz = clock_get_hz(clk_sys);
    uint32_t cyclesPerBit = clockHz / baud;
    int found = 0;

    for (int pin = 0; pin < count; pin++)
    {
        if (pin == (int)txPin || (skipMask & (1u << pin)))
        {
            continue;
        }

        printf("\r     Testing GP%-2d ...", pin);
        fflush(stdout);

        gpio_init(txPin);
        gpio_set_pulls(txPin, false, true);
        gpio_init(pin);
        gpio_put(pin, 1);                       // UART idle level
        gpio_set_dir(pin, GPIO_OUT);

        uint32_t quietEdges = countEdgesOn(txPin, UART_RX_QUIET_MS);
        uartBitBangSend(pin, "\r\n", cyclesPerBit);
        uint32_t replyEdges = countEdgesOn(txPin, UART_RX_REPLY_MS);
        uartBitBangSend(pin, "\r\n", cyclesPerBit);
        replyEdges += countEdgesOn(txPin, UART_RX_REPLY_MS);

        releasePin(pin);

        if (replyEdges >= UART_RX_MIN_EDGES && replyEdges > quietEdges * 6 + UART_RX_MIN_EDGES)
        {
            printf("\r     " CLR_OK "[+]" OFF " GP%d looks like target RX (the target replied)%s", pin, EOL);
            found++;
        }
    }
    releasePin(txPin);
    printf("\r                          \r");
    return found;
}

void reconUartScan(void)
{
    reconTitle("UART FINDER");
    int count = readChannelCount();

    printf("%s     Press any key, then power-cycle or reset the target so it prints boot output.%s", EOL, EOL);
    fflush(stdout);
    getc(stdin);

    listenOnChannels(count);
    printf("%s     Step 1: listening for TX for %d seconds...%s%s", EOL, UART_SCAN_MS / 1000, EOL, EOL);
    fflush(stdout);

    uint32_t clockHz = clock_get_hz(clk_sys);
    captureEdges((1u << count) - 1, (clockHz / 1000) * UART_SCAN_MS);
    releaseChannels(count);

    int found = 0, active = 0;
    int txPin = -1;
    uint32_t txBaud = 0;
    uint32_t activeMask = 0;

    for (int pin = 0; pin < count; pin++)
    {
        if (capture[pin].edges == 0)
        {
            continue;
        }
        active++;
        activeMask |= 1u << pin;
        uint32_t baud = detectBaud(&capture[pin], clockHz);
        if (baud)
        {
            printf("     " CLR_OK "[+]" OFF " GP%d looks like target TX at %lu baud%s",
                   pin, (unsigned long)baud, EOL);
            if (txPin < 0)
            {
                txPin = pin;
                txBaud = baud;
            }
            found++;
        }
    }

    if (found == 0)
    {
        printf("     No UART transmit signal recognized (%d active channel(s)).%s", active, EOL);
        printf("     Try again while the target is booting or producing output.%s%s", EOL, EOL);
        return;
    }

    if (count < 2)
    {
        printf("%s     Connect more channels to search for the target's RX pin.%s%s", EOL, EOL, EOL);
        return;
    }

    printf("%s     Step 2 sends Enter on each quiet channel and watches GP%d for a reply.%s", EOL, txPin, EOL);
    printf("     " CLR_DIM "It drives those channels, so connect them only to the target's UART pins." OFF "%s", EOL);
    printf("     " CLR_DIM "Wait until the target has finished booting before you continue." OFF "%s%s", EOL, EOL);
    printf("     \033[1;33m?\033[0m Search for the target's RX pin? (y/n): ");
    fflush(stdout);
    int answer = getc(stdin);
    printf("%c%s%s", (answer >= 32 && answer < 127) ? answer : ' ', EOL, EOL);

    int rxFound = 0;
    if (answer == 'y' || answer == 'Y')
    {
        rxFound = findUartRx(count, (uint)txPin, txBaud, activeMask);
        if (rxFound == 0)
        {
            printf("     No RX pin found. The target may ignore input, or its console may be disabled.%s", EOL);
        }
        else if (rxFound > 1)
        {
            printf("     " CLR_DIM "More than one pin replied. Test each one in USB-to-serial mode." OFF "%s", EOL);
        }
    }

    printf("%s     Wire target TX to GP1%s, then use USB-to-serial mode (U) at %lu baud.%s%s",
           EOL, rxFound ? " and target RX to GP0" : "", (unsigned long)txBaud, EOL, EOL);
}

//------------------------------------------ I2C scan ----------------------------------------------

static uint i2cSda;
static uint i2cScl;

static inline void lineLow(uint pin)
{
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
}

static inline void lineRelease(uint pin)
{
    gpio_set_dir(pin, GPIO_IN);
}

static inline void i2cDelay(void)
{
    busy_wait_us_32(I2C_HALF_US);
}

// Release SCL and honor clock stretching by the target.
static bool i2cClockHigh(void)
{
    lineRelease(i2cScl);
    for (uint32_t waited = 0; !gpio_get(i2cScl); waited++)
    {
        if (waited >= I2C_STRETCH_US)
        {
            return false;
        }
        busy_wait_us_32(1);
    }
    i2cDelay();
    return true;
}

static bool i2cClockBit(bool value, bool *sampled)
{
    if (value)
    {
        lineRelease(i2cSda);
    }
    else
    {
        lineLow(i2cSda);
    }
    i2cDelay();
    if (!i2cClockHigh())
    {
        return false;
    }
    if (sampled)
    {
        *sampled = gpio_get(i2cSda);
    }
    lineLow(i2cScl);
    i2cDelay();
    return true;
}

static void i2cStop(void)
{
    lineLow(i2cSda);
    i2cDelay();
    i2cClockHigh();
    lineRelease(i2cSda);
    i2cDelay();
}

// Clock out a stuck target, then confirm that both lines are idle high.
static bool i2cBusReady(void)
{
    lineRelease(i2cSda);
    lineRelease(i2cScl);
    i2cDelay();

    if (!gpio_get(i2cScl))
    {
        return false;
    }
    for (int i = 0; i < 9 && !gpio_get(i2cSda); i++)
    {
        lineLow(i2cScl);
        i2cDelay();
        if (!i2cClockHigh())
        {
            return false;
        }
    }
    if (!gpio_get(i2cSda))
    {
        return false;
    }
    i2cStop();
    return gpio_get(i2cSda) && gpio_get(i2cScl);
}

// Address the device for reading, then read one byte and NACK it.
static bool i2cProbe(uint8_t address, bool *busError)
{
    uint8_t header = (uint8_t)((address << 1) | 1);
    bool ackBit = true;

    lineLow(i2cSda);        // START
    i2cDelay();
    lineLow(i2cScl);
    i2cDelay();

    for (int bit = 7; bit >= 0; bit--)
    {
        if (!i2cClockBit((header >> bit) & 1, NULL))
        {
            *busError = true;
            return false;
        }
    }
    if (!i2cClockBit(true, &ackBit))
    {
        *busError = true;
        return false;
    }

    bool acknowledged = !ackBit;
    if (acknowledged)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            if (!i2cClockBit(true, NULL))
            {
                *busError = true;
                return true;
            }
        }
        if (!i2cClockBit(true, NULL))   // NACK ends the read
        {
            *busError = true;
            return true;
        }
    }
    i2cStop();
    return acknowledged;
}

void reconI2cScan(void)
{
    reconTitle("I2C SCANNER");
    printf("     SDA GPIO (0-%d) [0]: ", CHANNELS - 1);
    fflush(stdout);
    i2cSda = readNumber(0, CHANNELS - 1, 0);
    printf("     SCL GPIO (0-%d) [1]: ", CHANNELS - 1);
    fflush(stdout);
    i2cScl = readNumber(0, CHANNELS - 1, 1);

    if (i2cSda == i2cScl)
    {
        printf("%s     SDA and SCL must be different pins.%s%s", EOL, EOL, EOL);
        return;
    }

    gpio_init(i2cSda);
    gpio_init(i2cScl);
    gpio_set_pulls(i2cSda, true, false);
    gpio_set_pulls(i2cScl, true, false);
    sleep_us(100);

    if (!i2cBusReady())
    {
        printf("%s     Bus is not idle: SDA or SCL is being held low. Check the wiring.%s%s", EOL, EOL, EOL);
        releasePin(i2cSda);
        releasePin(i2cScl);
        return;
    }

    int found = 0;
    bool busError = false;

    printf("%s        ", EOL);
    for (int column = 0; column < 16; column++)
    {
        printf("%2x ", column);
    }
    for (int address = 0; address <= I2C_LAST_ADDR && !busError; address++)
    {
        if (address % 16 == 0)
        {
            printf("%s    %02x: ", EOL, address);
        }
        if (address < I2C_FIRST_ADDR)
        {
            printf("   ");
        }
        else if (i2cProbe((uint8_t)address, &busError))
        {
            printf("%02x ", address);
            found++;
        }
        else
        {
            printf("-- ");
        }
        fflush(stdout);
    }
    printf("%s%s", EOL, EOL);

    if (busError)
    {
        printf("     Scan stopped: a device held SCL low for too long.%s", EOL);
    }
    printf("     Devices found: %d%s%s", found, EOL, EOL);

    releasePin(i2cSda);
    releasePin(i2cScl);
}

//------------------------------------------ SPI flash ID ------------------------------------------

// SPI mode 0, most significant bit first.
static uint8_t spiTransfer(uint8_t output)
{
    uint8_t input = 0;

    for (int bit = 7; bit >= 0; bit--)
    {
        gpio_put(SPI_MOSI, (output >> bit) & 1);
        busy_wait_us_32(SPI_HALF_US);
        gpio_put(SPI_CLK, 1);
        input = (uint8_t)((input << 1) | (gpio_get(SPI_MISO) ? 1 : 0));
        busy_wait_us_32(SPI_HALF_US);
        gpio_put(SPI_CLK, 0);
    }
    return input;
}

void reconSpiFlashId(void)
{
    uint8_t id[SPI_ID_LEN];

    reconTitle("SPI FLASH IDENTIFICATION");
    printf("     Wiring: GPIO %d=CS, GPIO %d=CLK, GPIO %d=MOSI/DI, GPIO %d=MISO/DO, GND=GND%s",
           SPI_CS, SPI_CLK, SPI_MOSI, SPI_MISO, EOL);
    printf("     Press any key to read the chip ID...%s%s", EOL, EOL);
    fflush(stdout);
    getc(stdin);

    gpio_init(SPI_CS);
    gpio_init(SPI_CLK);
    gpio_init(SPI_MOSI);
    gpio_init(SPI_MISO);
    gpio_put(SPI_CS, 1);
    gpio_put(SPI_CLK, 0);
    gpio_put(SPI_MOSI, 0);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_set_dir(SPI_CLK, GPIO_OUT);
    gpio_set_dir(SPI_MOSI, GPIO_OUT);
    gpio_set_pulls(SPI_MISO, true, false);
    sleep_us(50);

    gpio_put(SPI_CS, 0);
    busy_wait_us_32(SPI_HALF_US);
    spiTransfer(0x9F);                      // JEDEC Read Identification
    for (int i = 0; i < SPI_ID_LEN; i++)
    {
        id[i] = spiTransfer(0x00);
    }
    busy_wait_us_32(SPI_HALF_US);
    gpio_put(SPI_CS, 1);

    releasePin(SPI_CS);
    releasePin(SPI_CLK);
    releasePin(SPI_MOSI);
    releasePin(SPI_MISO);

    int bank = 0;
    while (bank < SPI_ID_LEN - 3 && id[bank] == 0x7F)
    {
        bank++;
    }

    uint8_t manufacturer = id[bank];
    uint8_t type = id[bank + 1];
    uint8_t capacity = id[bank + 2];

    if (manufacturer == 0x00 || manufacturer == 0xFF || manufacturer == 0x7F)
    {
        printf("     No valid ID received (read 0x%02X). Check wiring and chip power.%s", manufacturer, EOL);
        printf("     In-circuit chips may need the host processor held in reset.%s%s", EOL, EOL);
        return;
    }

    const char *name = "Unknown";
    if (__builtin_parity(manufacturer))     // JEP106 codes have odd parity
    {
        name = jep106_table_manufacturer(bank, manufacturer & 0x7F);
    }

    printf("     JEDEC ID     : %02X %02X %02X%s", manufacturer, type, capacity, EOL);
    printf("     Manufacturer : %s%s", name, EOL);
    if (capacity >= 0x10 && capacity <= 0x19)
    {
        uint32_t kilobytes = (1u << capacity) / 1024;
        if (kilobytes >= 1024)
        {
            printf("     Capacity     : %lu MB (typical encoding)%s", (unsigned long)(kilobytes / 1024), EOL);
        }
        else
        {
            printf("     Capacity     : %lu KB (typical encoding)%s", (unsigned long)kilobytes, EOL);
        }
    }
    else
    {
        printf("     Capacity     : Unknown code 0x%02X%s", capacity, EOL);
    }
    printf("%s", EOL);
}
