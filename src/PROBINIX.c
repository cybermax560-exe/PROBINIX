/* 
    [ PROBINIX - Hardware hacker's multi-tool based on RP2040 dev boards ]

        Includes code by Aodrulez, used under the MIT License (see LICENSE).
        Inspired by JTAGulator. 

    [References & special thanks]
        https://github.com/grandideastudio/jtagulator
        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
        https://github.com/jbentham/picoreg
        https://github.com/szymonh/SWDscan
        Yusufss4 (https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)
        
        Flashrom support : 
            https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html
            https://github.com/stacksmashing/pico-serprog

        Openocd support  : 
            http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/
            http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/
            https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin
            https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c

        USB-to-Serial support :
            https://github.com/xxxajk/pico-uart-bridge
            https://github.com/Noltari/pico-uart-bridge

        CMSIS-DAP support :
            https://github.com/majbthrd/DapperMime
            https://github.com/raspberrypi/debugprobe
*/

#include "PROBINIX.h"
#include <string.h>
#include "pico/unique_id.h"

const char *banner=R"banner(
                      ___  ___  ____  ___  _____  _______  __
                     / _ \/ _ \/ __ \/ _ )/  _/ |/ /  _/ |/_/
                    / ___/ , _/ /_/ / _  |/ //    // /_>  <
                   /_/  /_/|_|\____/____/___/_/|_/___/_/|_|)banner";


char *version="1.0";
#define MAX_DEVICES_LEN      32                             // Maximum number of devices allowed in a single JTAG chain
#define MIN_IR_LEN           2                              // Minimum length of instruction register per IEEE Std. 1149.1
#define MAX_IR_LEN           32                             // Maximum length of instruction register
#define MAX_IR_CHAIN_LEN     MAX_DEVICES_LEN * MAX_IR_LEN   // Maximum total length of JTAG chain w/ IR selected
#define MAX_DR_LEN           4096                           // Maximum length of data register
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))
#define USB_HOST_RECOGNISE_TIME   (1000 * 1000 * 5)
#define USB_MODE_DEFAULT     0
#define USB_MODE_CMSISDAP    1
#define USB_MODE_UART        2
#define CR		             13
#define LF		             10
#define EOL                  "\r\n"
#define OFF                  "\033[0m"
#define BOLD                 "\033[1m"


#define onboardLED    25
#define unusedGPIO    21                               // Pins on Pico are accessed using GPIO names
#define MAX_NUM_JTAG  32
#define maxChannels   16                               // Max number of channels supported by Pico  

// Hardware boot mode pins
#define hwBootUSB2SerialPin  26
#define hwBootOpenocdPin     27
#define hwBootCmsisdapPin    28

bool isChannelSafe[maxChannels];
volatile int usbMode    = USB_MODE_DEFAULT;
uint progressCount      = 0;
uint maxPermutations    = 0;
char cmd;

uint jTDI;           
uint jTDO;
uint jTCK;
uint jTMS;
uint jTRST;
uint jDeviceCount;
bool jPulsePins;
uint32_t deviceIDs[MAX_DEVICES_LEN];                         // Array to store identified device IDs

uint xTDI;           
uint xTDO;
uint xTCK;
uint xTMS;
uint xTRST;

// function declarations (Needs clean-up later)
void resetUART(void);

// include file from openocd/src/helper
static const char * const jep106[][126] = {
#include "jep106.inc"
};

long int strtol(const char *str, char **endptr, int base);

// ------------------------------------------ Console UI ------------------------------------------

#define UI_WIDTH    76
#define UI_INDENT   "  "
#define CLR_HEAD    "\033[1;36m"
#define CLR_KEY     "\033[1;33m"
#define CLR_DIM     "\033[90m"
#define CLR_WARN    "\033[1;33m"

static void uiRule(char ch)
{
    printf("%s" CLR_DIM, UI_INDENT);
    for (int i = 0; i < UI_WIDTH; i++)
    {
        putchar(ch);
    }
    printf(OFF "%s", EOL);
}

static void uiCentered(const char *color, const char *text)
{
    int pad = (UI_WIDTH - (int)strlen(text)) / 2;
    if (pad < 0)
    {
        pad = 0;
    }
    printf("%s%*s%s%s" OFF "%s", UI_INDENT, pad, "", color, text, EOL);
}

#if defined(PICO_RP2350) && PICO_RP2350
#define UI_CHIP     "RP2350"
#else
#define UI_CHIP     "RP2040"
#endif

static void uiSection(const char *title, const char *hint)
{
    int used = (int)strlen(title) + 1;

    printf("%s%s" CLR_HEAD "%s" OFF, EOL, UI_INDENT, title);
    if (hint)
    {
        printf(" " CLR_DIM "(%s)" OFF, hint);
        used += (int)strlen(hint) + 3;
    }
    printf(" " CLR_DIM);
    for (int i = used; i < UI_WIDTH; i++)
    {
        putchar('-');
    }
    printf(OFF "%s", EOL);
}

static void uiGroup(const char *name)
{
    printf("%s" BOLD "%s" OFF "%s", UI_INDENT, name, EOL);
}

static void uiCommands(char key1, const char *text1, char key2, const char *text2)
{
    printf("%s  " CLR_KEY "%c" OFF "  %-34s", UI_INDENT, key1, text1);
    if (key2)
    {
        printf(CLR_KEY "%c" OFF "  %s", key2, text2);
    }
    printf("%s", EOL);
}

static void uiPinLine(const char *name, const char *pins)
{
    printf("%s  " "\033[1;32m" "%-6s" OFF "%s%s", UI_INDENT, name, pins, EOL);
}

static void uiPinout(void)
{
    uiSection("PINOUT", NULL);
    uiPinLine("UART", "GP0 = TX     GP1 = RX");
    uiPinLine("JTAG", "GP0 = TCK    GP1 = TMS    GP2 = TDI    GP3 = TDO");
    uiPinLine("SWD",  "GP2 = SWCLK  GP3 = SWDIO");
    uiPinLine("SPI",  "GP1 = CS     GP2 = CLK    GP3 = MOSI   GP4 = MISO");
    uiPinLine("I2C",  "GP0 = SDA    GP1 = SCL");
}

#define CLR_OK      "\033[1;32m"
#define CLR_FAIL    "\033[1;31m"

static void uiStatus(const char *color, const char *tag, const char *message)
{
    printf("%s%s%s" OFF " %s%s", UI_INDENT, color, tag, message, EOL);
}

#define uiOk(message)    uiStatus(CLR_OK, "[+]", message)
#define uiFail(message)  uiStatus(CLR_FAIL, "[-]", message)
#define uiInfo(message)  uiStatus(CLR_HEAD, "[*]", message)

static void uiWireHeader(void)
{
    printf("%s  " CLR_DIM "%-12s    %s" OFF "%s", UI_INDENT, "PROBINIX", "TARGET", EOL);
}

static void uiWire(const char *probinixPin, const char *targetPin)
{
    printf("%s  " CLR_KEY "%-12s" OFF CLR_DIM "->  " OFF "%s%s", UI_INDENT, probinixPin, targetPin, EOL);
}

static void uiNote(const char *message)
{
    printf("%s  " CLR_DIM "%s" OFF "%s", UI_INDENT, message, EOL);
}

static void uiCommand(const char *label, const char *command)
{
    printf("%s  " BOLD "%-7s" OFF " %s%s", UI_INDENT, label, command, EOL);
}

static bool uiAskYes(const char *question)
{
    printf("%s" CLR_KEY "?" OFF " %s (y/n): ", UI_INDENT, question);
    fflush(stdout);
    cmd = getc(stdin);
    printf("%c%s%s", (cmd >= 32 && cmd < 127) ? cmd : ' ', EOL, EOL);
    return cmd == 'y' || cmd == 'Y';
}

static void uiBoardId(char *id, size_t length)
{
    pico_get_unique_board_id_string(id, length);
}

static unsigned long uiClockMhz(void)
{
    return (unsigned long)(clock_get_hz(clk_sys) / 1000000);
}

static void uiTitleBar(void)
{
    char left[32];
    char right[64];
    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

    uiBoardId(id, sizeof(id));
    snprintf(left, sizeof(left), " PROBINIX v%s", version);
    snprintf(right, sizeof(right), "%s  %lu MHz  ID %s ", UI_CHIP, uiClockMhz(), id);

    int gap = UI_WIDTH - (int)strlen(left) - (int)strlen(right);
    if (gap < 1)
    {
        gap = 1;
    }
    printf("%s" "\033[30;46m" "%s%*s%s" OFF "%s", UI_INDENT, left, gap, "", right, EOL);
}

static void uiInfoRow(const char *label, const char *value)
{
    printf("%s  " CLR_DIM "%-13s" OFF "%s%s", UI_INDENT, label, value, EOL);
}

static void uiSystemInfo(void)
{
    char value[48];
    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

    uiSection("SYSTEM INFO", NULL);
    snprintf(value, sizeof(value), "PROBINIX %s", version);
    uiInfoRow("Firmware", value);
    uiInfoRow("Author", "BugFox");
    snprintf(value, sizeof(value), "%s at %lu MHz", UI_CHIP, uiClockMhz());
    uiInfoRow("Processor", value);
    uiBoardId(id, sizeof(id));
    uiInfoRow("Board ID", id);
    uiInfoRow("Build", __DATE__ " " __TIME__);
    uiInfoRow("Pin pulsing", jPulsePins ? "ON" : "OFF");
    printf("%s", EOL);
}

void splashScreen(void)
{
    printf("\033[2J\033[H");
    uiTitleBar();
    printf("\033[1;32m%s" OFF "%s%s", banner, EOL, EOL);
    uiCentered(BOLD, "Hardware hacking multi-tool for RP2040 boards");
    uiCentered(CLR_DIM, "by BugFox");
    printf("%s", EOL);
    uiRule('=');
}

void showPrompt(void)
{
    printf("%s" CLR_HEAD "probinix" OFF "> ", UI_INDENT);
    fflush(stdout);
}

void showMenu(void)
{
    char pulse[40];
    snprintf(pulse, sizeof(pulse), "Toggle pin pulsing [%s]", jPulsePins ? "ON" : "OFF");

    uiSection("COMMANDS", NULL);
    uiGroup("General");
    uiCommands('h', "Show this menu", 'v', "Show system info");
    printf("%s", EOL);
    uiGroup("Debug interface discovery");
    uiCommands('j', "Scan for JTAG pinout", 's', "Scan for SWD pinout");
    uiCommands('p', pulse, 0, NULL);
    printf("%s", EOL);
    uiGroup("Target reconnaissance");
    uiCommands('m', "Monitor pin activity", 'u', "Find UART TX, RX and baud");
    uiCommands('i', "Scan I2C bus", 'f', "Identify SPI flash chip");
    printf("%s", EOL);
    uiGroup("Hardware modes (unplug or reset to exit)");
    uiCommands('U', "USB-to-serial bridge", 'F', "Flashrom SPI programmer");
    uiCommands('O', "OpenOCD JTAG/SWD adapter", 'C', "CMSIS-DAP v1.0 debugger");

    uiPinout();

    uiSection("BOOT MODES", "Connect one pin to GND, then reset");
    printf("%s  " CLR_KEY "GPIO 26" OFF "  USB-to-serial     "
           CLR_KEY "GPIO 27" OFF "  OpenOCD     "
           CLR_KEY "GPIO 28" OFF "  CMSIS-DAP%s", UI_INDENT, EOL);

    uiSection("NOTES", NULL);
    printf("%s  - Disable local echo in your terminal.%s", UI_INDENT, EOL);
    printf("%s  - If no JTAG or SWD pinout is found, toggle pin pulsing (p) and retry.%s", UI_INDENT, EOL);
    printf("%s  " CLR_WARN "! 3.3 V logic only. Keep GPIO 0-15 disconnected from GND while scanning." OFF "%s%s",
           UI_INDENT, EOL, EOL);
}

void printProgress(size_t count, size_t max) {
    const int bar_width = 50;

    float progress = (max > 0) ? (float) count / max : 1.0f;
    if (progress > 1.0f)
    {
        progress = 1.0f;
    }
    int bar_length = progress * bar_width;

    printf("\r%s  " CLR_DIM "[" OFF CLR_HEAD, UI_INDENT);
    for (int i = 0; i < bar_length; ++i) {
        putchar('#');
    }
    printf(OFF CLR_DIM);
    for (int i = bar_length; i < bar_width; ++i) {
        putchar('.');
    }
    printf("]" OFF " %5.1f%%", progress * 100);

    fflush(stdout);
}

int stringToInt(char * str)
{
   char *endptr;
   long int num;
   int res = 0;
   num = strtol(str, &endptr, 10);
   if (endptr == str) 
   {
      return 0;
   } 
   else if (*endptr != '\0') 
   {
      return 0;
   } 
   else 
   {
      return((int)num);
   }
   return 0;
}

int getIntFromSerial(void)
{
    char strg[3] = {0, 0, 0};
    char chr;
    int lp = 0;
    int value = 0;
    chr = getc(stdin);
    printf("%c",chr);
    if(chr == CR || chr == LF || chr < 48 || chr > 57)
    {
        value = 0;
    }
    else if (chr > 49) 
    {
      strg[0] = chr;
      value = stringToInt(strg);
    }
    else 
    {
      strg[0] = chr;
      chr = getc(stdin);
      printf("%c",chr);
      if(chr == CR || chr == LF || chr < 48 || chr > 57)
      {
          strg[1] = 0;
      }
      else
      {
          strg[1] = chr;
      }
      value = stringToInt(strg);       
    }
    printf("%s", EOL);
    return(value);
}

int getChannels(void)
{
    int x;
    printf("%s" CLR_KEY "?" OFF " Channels connected, starting at GP0 (4-%d): ", UI_INDENT, maxChannels);
    x = getIntFromSerial();
    while(x < 4 || x > maxChannels)
    {
        printf("%s  Enter a value from 4 to %d: ", UI_INDENT, maxChannels);
        x = getIntFromSerial();
    }
    printf("%s" CLR_HEAD "[*]" OFF " Scanning with %d channels...%s%s", UI_INDENT, x, EOL, EOL);
    return(x);
}

void pulsePins(int channelCount)
{
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        gpio_pull_up(x);
        gpio_pull_down(x);
    }   
}

// Initialize all available channels & set them as output
void initChannels(void)
{
    for(int x=0; x < maxChannels ; x++)
    {
        gpio_init(x);
        gpio_set_dir(x, GPIO_IN);
    }   
}

void jtagInitChannels(int channelCount)
{
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        if (jPulsePins)
        {
            gpio_pull_down(x);
        }
        else
        {
            gpio_pull_up(x);
        }
    }   
}

void jtagConfig(uint tdiPin, uint tdoPin, uint tckPin, uint tmsPin)
{
    // Output
    gpio_set_dir(tdiPin, GPIO_OUT);
    gpio_set_dir(tckPin, GPIO_OUT);
    gpio_set_dir(tmsPin, GPIO_OUT);

    // Input
    gpio_set_dir(tdoPin, GPIO_IN);
    gpio_put(tckPin, 0);
}

void jtagResetChannels(int channelCount)
{
    // Input to be safe
    for(int x=0; x < channelCount ; x++)
    {
        gpio_set_dir(x, GPIO_IN);
        if (xTRST > 0 && xTRST == x)    //Skip altering state of TRST if found
        {
            gpio_set_dir(x, GPIO_IN);
            if (jPulsePins)
            {
                gpio_pull_down(jTRST);
            }
            else
            {
                gpio_pull_up(jTRST);          
            }            
        }
    }   
}

// Generate one TCK pulse. Read TDO inside the pulse.
// Expects TCK to be low upon being called.
bool tdoRead(void)
{
    bool volatile tdoStatus;
    gpio_put(jTCK, 1);
    tdoStatus=gpio_get(jTDO);
    gpio_put(jTCK, 0);
    return(tdoStatus);
}

// Generates on TCK Pulse
// Expects TCK to be low when called & ignores TDO
void tckPulse(void)
{
    bool tdoStatus;
    tdoStatus=tdoRead();
}

void tdiHigh(void)
{
    gpio_put(jTDI, 1);
}

void tdiLow(void)
{
    gpio_put(jTDI, 0);
}

void tmsHigh(void)
{
    gpio_put(jTMS, 1);
}

void tmsLow(void)
{
    gpio_put(jTMS, 0);
}

void restoreIdle(void)
{
    tmsHigh();
    for(int x=0; x < 5; x++)    // 5 is sufficient, adding few more to be sure
    {
        tckPulse();
    }
    tmsLow();
    tckPulse();                 // Got to Run-Test-Idle
}

void enterShiftDR(void)
{
    tmsHigh();
    tckPulse();                 // Go to Select DR
    tmsLow();
    tckPulse();                 // Go to Capture DR
    tmsLow();
    tckPulse();                 // Go to Shift DR
}

void enterShiftIR(void)
{
    tmsHigh();
    tckPulse();                 // Go to Select DR
    tmsHigh();
    tckPulse();                 // Go to Select IR
    tmsLow();
    tckPulse();                 // Go to Capture IR
    tmsLow();
    tckPulse();                 // Go to Shift IR
}

uint32_t bitReverse(uint32_t n)
{
    uint32_t reversed = 0;
    for (int i = 0; i < 32; i++) 
    {
        reversed <<= 1;           // Shift reversed bits to the left
        reversed |= (n & 1);      // Add the least significant bit of n to reversed
        n >>= 1;                  // Shift n to the right
    }
    return reversed;
}

void getDeviceIDs(int number)
{
    restoreIdle();              // Reset TAP to Run-Test-Idle
    enterShiftDR();             // Go to Shift DR

    tdiHigh();
    tmsLow();
    uint32_t tempValue;
    for(int x=0; x < number;x++)
    {  
        tempValue=0;
        for(int y=0; y<32; y++)
        {
            tempValue <<= 1;
            tempValue |= tdoRead();
        }
        tempValue = bitReverse(tempValue);
        deviceIDs[x]=tempValue;
    }

    restoreIdle();              // Reset TAP to Run-Test-Idle
}

void displayPinout(void)
{
    printProgress(maxPermutations, maxPermutations);
    printf("%s%s", EOL, EOL);
    uiOk("JTAG pinout found");
    printf("%s  TDI = " CLR_KEY "GP%u" OFF "   TDO = " CLR_KEY "GP%u" OFF "   TCK = " CLR_KEY "GP%u" OFF
           "   TMS = " CLR_KEY "GP%u" OFF "   TRST = ", UI_INDENT, xTDI, xTDO, xTCK, xTMS);
    if(xTRST != 0)
    {
        printf(CLR_KEY "GP%u" OFF "%s%s", xTRST, EOL, EOL);
    }
    else
    {
        printf(CLR_DIM "not found" OFF "%s%s", EOL, EOL);
    }
}

const char *jep106_table_manufacturer(unsigned int bank, unsigned int id)
{
	if (id < 1 || id > 126) 
    {
		return "Unknown";
	}
	/* index is zero based */
	id--;
	if (bank >= ARRAY_SIZE(jep106) || jep106[bank][id] == 0)
		return "Unknown";
	return jep106[bank][id];
}

bool isValidDeviceID(uint32_t idc)
{        
    long part = (idc & 0xffff000) >> 12;
    int bank=(idc & 0xf00) >> 8;
    int id=(idc & 0xfe) >> 1;
    int ver=(idc & 0xf0000000) >> 28;

    if (id > 1 && id <= 126 && bank <= 8 && idc > 0) 
    {
        return(true);
    }

    return(false);
}

static void uiPrintDevice(int index, uint32_t idc)
{
    unsigned long part = (idc & 0xffff000) >> 12;
    int bank = (idc & 0xf00) >> 8;
    int id = (idc & 0xfe) >> 1;
    int ver = (idc & 0xf0000000) >> 28;

    printf("%s  Device %d   IDCODE " CLR_KEY "0x%08lX" OFF, UI_INDENT, index, (unsigned long)idc);
    if (id > 1 && id <= 126 && bank <= 8)
    {
        printf("   %s, part 0x%04lX, rev %d", jep106_table_manufacturer(bank, id), part, ver);
    }
    printf("%s", EOL);
}

void displayDeviceDetails(void)
{
    for(int x=0; x < jDeviceCount; x++)
    {
        uiPrintDevice(x, deviceIDs[x]);
    }
    printf("%s", EOL);
    if (uiAskYes("Start OpenOCD JTAG mode with this pinout?"))
    {
        uiNote("Example command. Replace COM4 with your PROBINIX port.");
        uiCommand("JTAG", "openocd -f interface/buspirate.cfg -c \"buspirate port COM4; transport select jtag;\" -f target/esp32.cfg");
        printf("%s", EOL);
        resetUART();
        initOpenocdMode(jTCK, jTMS, jTDI, jTDO, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_MODE_JTAG);
    }
}

// Function to detect number of devices in the scan chain 
int detectDevices(void)
{
    int volatile x;
    restoreIdle();
    enterShiftIR();

    tdiHigh();
    for(x = 0; x < MAX_IR_CHAIN_LEN; x++)
    {
        tckPulse();
    }

    tmsHigh();
    tckPulse();     //Go to Exit1 IR

    tmsHigh();
    tckPulse();     //Go to Update IR, new instruction in effect

    tmsHigh();
    tckPulse();     //Go to Select DR

    tmsLow();
    tckPulse();     //Go to Capture DR

    tmsLow();
    tckPulse();     //Go to Shift DR

    for(x = 0; x < MAX_DEVICES_LEN; x++)
    {
        tckPulse();
    }

    // We are now in BYPASS mode with all DR set
    // Send in a 0 on TDI and count until we see it on TDO
    tdiLow();
    for(x = 0; x < (MAX_DEVICES_LEN - 1); x++)
    {
        if(tdoRead() == false)
        {
            break;                      // Our 0 has propagated through the entire chain
                                        // 'x' holds the number of devices
        }
    }

    if (x > (MAX_DEVICES_LEN - 1))
    {
        x = 0;
    }
    
    tmsHigh();
    tckPulse();
    tmsHigh();
    tckPulse();
    tmsLow();
    tckPulse();                         // Go to Run-Test-Idle
    return(x);
}

uint32_t shiftArray(uint32_t array, int numBits)
{
    uint32_t tempData;
    int x;
    tempData=0;

    for(x=1;x <= numBits; x++)
    {
        if( x == numBits)
        {
          tmsHigh();
        }  

        if (array & 1)
            {tdiHigh();}
        else
            {tdiLow();}

        array >>= 1 ;
        tempData <<= 1;
        tempData |=tdoRead();
    }
    return(tempData);
}

uint32_t sendData(uint32_t pattern, int num)
{
    uint32_t tempData;
    tempData=0;
    enterShiftDR();
    tempData=shiftArray(pattern, num);
    tmsHigh();
    tckPulse();             // Go to Update DR, new data in effect

    tmsLow();
    tckPulse();             // Go to Run-Test-Idle
    
    return(tempData);
}

uint32_t bypassTest(int num, uint32_t bPattern)
{
    if(num <= 0 || num > MAX_DEVICES_LEN)   // Discard false-positives
    {
        return(0);
    }

    int x;
    uint32_t value;
    restoreIdle();
    enterShiftIR();

    tdiHigh();
    for(x=0; x < (num * MAX_IR_LEN); x++)      // Send in 1s
    {
        tckPulse();
    }

    tmsHigh();
    tckPulse();               // Go to Exit1 IR

    tmsHigh();
    tckPulse();              // Go to Update IR, new instruction in effect

    tmsLow();
    tckPulse();              // Go to Run-Test-Idle       

    value=sendData(bPattern, 32 + num); // This is correct, verified.
    value=bitReverse(value);
    return(value);

}

uint32_t uint32Rand(void)
{
  static uint32_t Z;
  if (Z & 1) {
    Z = (Z >> 1);
  } else {
    Z = (Z >> 1) ^ 0x7FFFF159;
  }
  return (Z);
}

int calculateJtagPermutations(int totalChannels) 
{
    int result = 1;
    for (int i = 0; i < 4; i++)             // Minimum required pins == 4
    {
        result *= (totalChannels - i);
    }
    return result;
}

void jtagScan(void)
{
    int channelCount;
    uint32_t tempDeviceId;
    bool volatile foundPinout=false;
    jDeviceCount=0;
    channelCount = getChannels();            // First get the number of channels hooked
    progressCount = 0;
    maxPermutations = calculateJtagPermutations(channelCount+1);
    jTDO, jTCK, jTMS, jTDI,jTRST = 0;
    for(jTDI=0; jTDI <= channelCount; jTDI++)
    {
        for(jTDO=0; jTDO <= channelCount; jTDO++)
        {
            if (jTDI == jTDO)
            {
                continue;
            }
            for(jTCK =0; jTCK  <= channelCount; jTCK++)
            {
                if (jTCK  == jTDO || jTCK == jTDI)
                {
                    continue;
                }
                for(jTMS=0; jTMS <= channelCount; jTMS++)
                {                      
                        if (jTMS == jTCK || jTMS == jTDO || jTMS == jTDI)
                        {
                            continue;
                        }
                        // onBoard LED notification
                        gpio_put(onboardLED, 0);
                        
                        progressCount = progressCount+1;
                        printProgress(progressCount, maxPermutations);
                        jtagInitChannels(channelCount);                      
                        if (jPulsePins)
                        {
                            pulsePins(channelCount);
                        }
                        jtagConfig(jTDI, jTDO, jTCK, jTMS);
                        jDeviceCount=detectDevices();
                        
                        uint32_t dataIn;
                        uint32_t dataOut;
                        dataIn=uint32Rand();
                        dataOut=bypassTest(jDeviceCount, dataIn);          
                        if(dataIn == dataOut)
                        {
                            jDeviceCount=detectDevices();
                            getDeviceIDs(jDeviceCount);
                            tempDeviceId=deviceIDs[0];
                            if (isValidDeviceID(tempDeviceId) == false || jDeviceCount <= 0 )
                            {
                              continue;
                            }
                            else
                            {
                              foundPinout=true;
                            }
                            
                            // Found all pins except nTRST, so let's try
                            xTDI=jTDI;
                            xTDO=jTDO;
                            xTCK=jTCK;
                            xTMS=jTMS;
                            xTRST=0;
                            for(jTRST=0; jTRST <= channelCount; jTRST++)
                            {
                                if (jTRST == jTMS || jTRST == jTCK || jTRST == jTDO || jTRST == jTDI)
                                {
                                    continue;
                                }
                                progressCount = progressCount+1;
                                printProgress(progressCount, maxPermutations);
                                
                                gpio_set_dir(jTRST, GPIO_IN);
                                if (jPulsePins)
                                {
                                    gpio_pull_up(jTRST);
                                }
                                else
                                {
                                    gpio_pull_down(jTRST);
                                }

                                sleep_us(10);          // Give device time to react

                                getDeviceIDs(1);
                                if (tempDeviceId != deviceIDs[0] )
                                {
                                    deviceIDs[0]=tempDeviceId;
                                    xTRST=jTRST;
                                    break;
                                }
                            }
                            // Done enumerating everything. 
                            if(foundPinout==true)
                            {
                              jtagResetChannels(channelCount);
                              displayPinout();
                              displayDeviceDetails();
                              // onBoard LED notification
                              gpio_put(onboardLED, 1);
                              return;
                            }                            
                        }
                        // onBoard LED notification
                        jtagResetChannels(channelCount);
                        gpio_put(onboardLED, 1);
                    }
            }
        }
    }
    if( foundPinout == false )
    {
        printProgress(maxPermutations, maxPermutations);
        printf("%s%s", EOL, EOL);
        uiFail("No JTAG device found. Check wiring and GND, or toggle pin pulsing (p).");
        printf("%s", EOL);
    }

}


//-------------------------------------SWD Scan [custom implementation]-----------------------------
     
#define LINE_RESET_CLK_CYCLES 52        // Atleast 50 cycles, selecting 52 
#define LINE_RESET_CLK_IDLE_CYCLES 2    // For Line Reset, have to send both of these
#define SWD_DELAY 5
#define JTAG_TO_SWD_CMD 0xE79E
#define SWD_TO_JTAG_CMD 0xE73C
#define SWDP_ACTIVATION_CODE 0x1A
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

uint xSwdClk=0;
uint xSwdIO=1;
bool swdDeviceFound=false;
int getSwdChannels(void)
{
    int x;
    printf("%s" CLR_KEY "?" OFF " Channels connected, starting at GP0 (2-%d): ", UI_INDENT, maxChannels);
    x = getIntFromSerial();
    while(x < 2 || x > maxChannels)
    {
        printf("%s  Enter a value from 2 to %d: ", UI_INDENT, maxChannels);
        x = getIntFromSerial();
    }
    printf("%s" CLR_HEAD "[*]" OFF " Scanning with %d channels...%s%s", UI_INDENT, x, EOL, EOL);
    return(x);
}

void swdDisplayDeviceDetails(uint32_t idcode)
{
    uiPrintDevice(0, idcode);
    printf("%s", EOL);
    if (uiAskYes("Start OpenOCD SWD mode with this pinout?"))
    {
        uiNote("Example command. Replace COM4 with your PROBINIX port.");
        uiCommand("SWD", "openocd -f interface/buspirate.cfg -c \"buspirate port COM4; transport select swd;\" -f target/stm32f1x.cfg");
        printf("%s", EOL);
        resetUART();
        initOpenocdMode(OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, OPENOCD_PIN_DEFAULT, xSwdClk, xSwdIO, OPENOCD_MODE_SWD);
    }
}

void swdDisplayPinout(int swdio, int swclk, uint32_t idcode)
{
    printProgress(maxPermutations, maxPermutations);
    printf("%s%s", EOL, EOL);
    uiOk("SWD pinout found");
    printf("%s  SWDIO = " CLR_KEY "GP%d" OFF "   SWCLK = " CLR_KEY "GP%d" OFF "%s%s", UI_INDENT, swdio, swclk, EOL, EOL);
    swdDisplayDeviceDetails(idcode);
}

void initSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_OUT);
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void resetSwdPins(void)
{
    gpio_set_dir(xSwdClk,GPIO_IN);
    gpio_set_dir(xSwdIO,GPIO_IN);
}

void swdClockPulse(void)
{
    gpio_put(xSwdClk, 0);
    sleep_us(SWD_DELAY);
    gpio_put(xSwdClk, 1);
    sleep_us(SWD_DELAY);
}

void swdSetReadMode(void)
{
    gpio_set_dir(xSwdIO,GPIO_IN);
}

void swdTurnAround(void)
{
    swdSetReadMode();
    swdClockPulse();
}

void swdSetWriteMode(void)
{
    gpio_set_dir(xSwdIO,GPIO_OUT);
}

void swdIOHigh(void)
{
    gpio_put(xSwdIO, 1);
}

void swdIOLow(void)
{
    gpio_put(xSwdIO, 0);
}

void swdWriteHigh(void)
{
    gpio_put(xSwdIO, 1);
    swdClockPulse();
}

void swdWriteLow(void)
{
    gpio_put(xSwdIO, 0);
    swdClockPulse();
}

bool swdReadBit(void)
{
    bool value=gpio_get(xSwdIO);
    swdClockPulse();
    return(value);
}

void swdReadDPIDR(void)
{
    long buffer;
    bool value;
    for(int x=0; x< 32; x++)
    {
        value=swdReadBit();
        bitWrite(buffer, x, value);
    }
    if (buffer > 0)
    {
        swdDisplayPinout(xSwdIO, xSwdClk, buffer);
    }
    else
    {
        swdDeviceFound=false;
    }
}

// Receive ACK response from SWD device & verify if OK
bool swdReadAck(void)
{
    bool bit1=swdReadBit();
    bool bit2=swdReadBit();
    bool bit3=swdReadBit();
    if(bit1 == true && bit2 == false && bit3 == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void swdWriteBit(bool value)
{
    gpio_put(xSwdIO, value);
    swdClockPulse();
}

void swdWriteBits(long value, int length)
{
    for (int i=0; i<length; i++)
    {
        swdWriteBit(bitRead(value, i));
    }
}

void swdResetLineSWDJ(void)
{
    swdIOHigh();
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        swdClockPulse();
    }
}

void swdResetLineSWD(void)
{
    swdIOHigh();
    for(int x=0; x < LINE_RESET_CLK_CYCLES+10; x++)
    {
        swdClockPulse();
    }
    swdIOLow();
    swdClockPulse();
    swdClockPulse();
    swdClockPulse();
    swdClockPulse();
    swdIOHigh();
}

// Leave dormant state
void swdArmWakeUp(void)
{
    swdSetWriteMode();
    swdIOHigh();
    for(int x=0;x < 8; x++)     // Reset to selection Alert Sequence
    {
        swdClockPulse();
    }

    // Send selection alert sequence 0x19BC0EA2 E3DDAFE9 86852D95 6209F392 (128 bits)
    swdWriteBits(0x92, 8);
    swdWriteBits(0xf3, 8);
    swdWriteBits(0x09, 8);
    swdWriteBits(0x62, 8);

    swdWriteBits(0x95, 8);
    swdWriteBits(0x2D, 8);
    swdWriteBits(0x85, 8);
    swdWriteBits(0x86, 8);

    swdWriteBits(0xE9, 8);
    swdWriteBits(0xAF, 8);
    swdWriteBits(0xDD, 8);
    swdWriteBits(0xE3, 8);

    swdWriteBits(0xA2, 8);
    swdWriteBits(0x0E, 8);
    swdWriteBits(0xBC, 8);
    swdWriteBits(0x19, 8);

    swdWriteBits(0x00, 4);   // idle bits
    swdWriteBits(SWDP_ACTIVATION_CODE, 8);
}

void swdToJTAG(void)
{
  swdResetLineSWDJ();
  swdWriteBits(SWD_TO_JTAG_CMD, 16);
}

void swdTrySWDJ(void)
{
    swdSetWriteMode();
    swdArmWakeUp();                     // Needed for devices like RPi Pico
    swdResetLineSWDJ();
    swdWriteBits(JTAG_TO_SWD_CMD, 16);
    swdResetLineSWDJ();
    swdWriteBits(0x00, 4);

    swdWriteBits(0xA5, 8);             // readIdCode command 0b10100101
    swdTurnAround();
    
    if(swdReadAck() == true)           // Got ACK OK
    {
        swdDeviceFound=true;
        swdReadDPIDR();
    }
    swdTurnAround();
    swdSetWriteMode();
    swdWriteBits(0x00, 8);
}

bool swdBruteForce(void)
{
    // onBoard LED notification
    gpio_put(onboardLED, 0);
    swdTrySWDJ();
    gpio_put(onboardLED, 1);
    if(swdDeviceFound)
    { 
        swdToJTAG();
        return(true); 
    } else 
    { 
        return(false);
    }
}

void swdScan(void)
{     
    swdDeviceFound = false;
    bool result = false;    
    int channelCount = getSwdChannels();    
    progressCount = 0;
    maxPermutations = channelCount * (channelCount + 1);   // Needs to be fixed
    for(uint clkPin=0; clkPin <= channelCount; clkPin++)
    {
        xSwdClk = clkPin;
        for(uint ioPin=0; ioPin <= channelCount; ioPin++)
        {
            xSwdIO = ioPin;
            if( xSwdClk == xSwdIO)
            {
                continue;
            }
            printProgress(progressCount, maxPermutations);
            progressCount++;
            initSwdPins();
            result = swdBruteForce();
            resetSwdPins();
            if (result) break;
        }
        if (result) break; 
    }
    if(swdDeviceFound == false)
    {
        printProgress(maxPermutations, maxPermutations);
        printf("%s%s", EOL, EOL);
        uiFail("No SWD device found. Check wiring and GND, or toggle pin pulsing (p).");
        printf("%s", EOL);
    }
    // Switch back to JTAG
    swdToJTAG();
}

//--------------------------------------------Main--------------------------------------------------

static void busyLoop(size_t count) 
{
    for (volatile size_t i = 0; i < count; i++);
}

void resetUART(void)
{
    printf("%s" CLR_KEY "?" OFF " Press any key to start this mode...%s", UI_INDENT, EOL);
    char cmd = getc(stdin);
    printf(BOLD);
    printf("%s" CLR_OK "[+]" OFF " Mode started. The USB device will now reconnect.%s%s", UI_INDENT, EOL, EOL);
    sleep_ms(4);
    printf(OFF);
    fflush(stdout);
    busyLoop(USB_HOST_RECOGNISE_TIME);
    tud_disconnect();
    busyLoop(USB_HOST_RECOGNISE_TIME);
    tud_connect();
}

void enableBootModePulls(void)
{
    // Set pull-ups to avoid false positives
    gpio_set_pulls (hwBootUSB2SerialPin, true, false);
    gpio_set_pulls (hwBootOpenocdPin, true, false);
    gpio_set_pulls (hwBootCmsisdapPin, true, false);
}

void disableBootModePulls(void)
{
    gpio_disable_pulls(hwBootCmsisdapPin);
    gpio_disable_pulls(hwBootUSB2SerialPin);
    gpio_disable_pulls(hwBootOpenocdPin);        
}


void hardwareModeBoot(void)
{
    // Init hardware boot mode pins
    gpio_init(hwBootUSB2SerialPin);
    gpio_init(hwBootOpenocdPin);
    gpio_init(hwBootCmsisdapPin);    

    // Setup hardware boot mode pins
    gpio_set_dir(hwBootUSB2SerialPin, GPIO_IN);
    gpio_set_dir(hwBootOpenocdPin, GPIO_IN);
    gpio_set_dir(hwBootCmsisdapPin, GPIO_IN);

    enableBootModePulls();

    sleep_us(50);
    if (gpio_get(hwBootCmsisdapPin) == false)
    {
        disableBootModePulls();
        usbMode = USB_MODE_CMSISDAP;
        cmsisDapInit();
    }
    else 
    {
        usbMode = USB_MODE_DEFAULT;
        
        if (gpio_get(hwBootUSB2SerialPin) == false)
        {
            disableBootModePulls();  
            usbMode = USB_MODE_UART;
            uartBootMode();
        }
        else if (gpio_get(hwBootOpenocdPin) == false)
        {
            disableBootModePulls();            
            initUART();
            stdio_usb_init();       
            jTCK = OPENOCD_PIN_DEFAULT;
            jTMS = OPENOCD_PIN_DEFAULT;
            jTDI = OPENOCD_PIN_DEFAULT;
            jTDO = OPENOCD_PIN_DEFAULT;
            xSwdClk = OPENOCD_PIN_DEFAULT;
            xSwdIO = OPENOCD_PIN_DEFAULT;
            initOpenocdMode(jTCK, jTMS, jTDI, jTDO, xSwdClk, xSwdIO, OPENOCD_MODE_GENERIC);
        }
        else
        {
            disableBootModePulls();
            initUART();
            stdio_usb_init();
        }
    }
}



int main()
{
    gpio_init(onboardLED);
    gpio_set_dir(onboardLED, GPIO_OUT);
    gpio_put(onboardLED, 1);
    initChannels();

    jPulsePins=false;

    // Check for Hardware modes
    hardwareModeBoot();
    
    //get user input to display splash & menu    
    cmd=getc(stdin);
    splashScreen();
    showMenu();
    showPrompt();

    while(1)
    {
        cmd=getc(stdin);
        if (cmd == CR || cmd == LF)     // Ignore line endings left by numeric prompts
        {
            continue;
        }
        printf("%c%s%s", cmd, EOL, EOL);
        switch(cmd)
        {
            // Help menu requested
            case 'h':
                splashScreen();
                showMenu();
                break;
            case 'v':
                uiSystemInfo();
                break;

            case 'j':
                jtagScan();
                break;

            case 's':
                swdScan();
                break;

            case 'p':
                jPulsePins=!jPulsePins;
                if(jPulsePins)
                {
                    uiOk("Pin pulsing ON");
                    printf("%s", EOL);
                }
                else
                {
                    uiOk("Pin pulsing OFF");
                    printf("%s", EOL);
                }                
                break;


            case 'l':
                for(int x=0;x<=25;x++)
                {
                    gpio_put(onboardLED, 1);
                    sleep_ms(250);
                    gpio_put(onboardLED, 0);
                    sleep_ms(250);
                }
                break;

            case 'm':
                reconPinMonitor();
                break;

            case 'u':
                reconUartScan();
                break;

            case 'i':
                reconI2cScan();
                break;

            case 'f':
                reconSpiFlashId();
                break;

            case 'U':
                uiSection("USB-TO-SERIAL MODE", NULL);
                uiWireHeader();
                uiWire("GP0 (TX)", "RX");
                uiWire("GP1 (RX)", "TX");
                uiWire("GND", "GND");
                uiWire("3V3 OUT", "VCC, only if the target has no power");
                printf("%s", EOL);
                uiNote("Reopen the COM port and set the target's UART settings.");
                printf("%s", EOL);

                usbMode = USB_MODE_UART;
                resetUART();
                fflush(stdout);
                busyLoop(USB_HOST_RECOGNISE_TIME);
                usbMode = USB_MODE_UART;
                stdio_set_driver_enabled(&stdio_usb, false);

                stdio_usb_deinit();
                uartMode();
                break;

            case 'F':
                uiSection("FLASHROM SPI PROGRAMMER", NULL);
                uiWireHeader();
                uiWire("GP1", "CS");
                uiWire("GP2", "CLK");
                uiWire("GP3", "MOSI / DI");
                uiWire("GP4", "MISO / DO");
                uiWire("GND", "GND");
                uiWire("3V3 OUT", "VCC, only if the target has no power");
                printf("%s", EOL);
                uiNote("Windows  (replace COM4 with your PROBINIX port)");
                uiCommand("Detect", "flashrom -p serprog:dev=COM4:115200,spispeed=4M");
                uiCommand("Read", "flashrom -p serprog:dev=COM4:115200,spispeed=4M -r backup.bin");
                uiCommand("Write", "flashrom -p serprog:dev=COM4:115200,spispeed=4M -w firmware.bin");
                printf("%s", EOL);
                uiNote("Linux  (find the port with: ls /dev/ttyACM*)");
                uiCommand("Detect", "flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M");
                uiCommand("Read", "flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M -r backup.bin");
                uiCommand("Write", "flashrom -p serprog:dev=/dev/ttyACM0:115200,spispeed=4M -w firmware.bin");
                printf("%s", EOL);
                uiNote("Close this terminal before running flashrom. Use spispeed=1M if reads fail.");
                printf("%s", EOL);
                resetUART();
                initSerProg();
                break;

            case 'O':
                uiSection("OPENOCD DEBUGGER MODE", "Bus Pirate protocol");
                uiWireHeader();
                uiWire("GP0", "TCK (JTAG)");
                uiWire("GP1", "TMS (JTAG)");
                uiWire("GP2", "TDI (JTAG)  or  SWCLK (SWD)");
                uiWire("GP3", "TDO (JTAG)  or  SWDIO (SWD)");
                uiWire("GND", "GND");
                printf("%s", EOL);
                uiNote("Example commands. Replace COM4 with your PROBINIX port.");
                uiCommand("JTAG", "openocd -f interface/buspirate.cfg -c \"buspirate port COM4; transport select jtag;\" -f target/esp32.cfg");
                uiCommand("SWD", "openocd -f interface/buspirate.cfg -c \"buspirate port COM4; transport select swd;\" -f target/stm32f1x.cfg");
                printf("%s", EOL);
                resetUART();                
     
                jTCK = OPENOCD_PIN_DEFAULT;
                jTMS = OPENOCD_PIN_DEFAULT;
                jTDI = OPENOCD_PIN_DEFAULT;
                jTDO = OPENOCD_PIN_DEFAULT;
                xSwdClk = OPENOCD_PIN_DEFAULT;
                xSwdIO = OPENOCD_PIN_DEFAULT;
                initOpenocdMode(jTCK, jTMS, jTDI, jTDO, xSwdClk, xSwdIO, OPENOCD_MODE_GENERIC);
                break;

            case 'C':
                uiSection("CMSIS-DAP v1.0 DEBUGGER", NULL);
                uiWireHeader();
                uiWire("GP0 (TX)", "UART RX");
                uiWire("GP1 (RX)", "UART TX");
                uiWire("GP2", "SWCLK");
                uiWire("GP3", "SWDIO");
                uiWire("GND", "GND");
                printf("%s", EOL);
                uiNote("Example command:");
                uiCommand("OpenOCD", "openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg");
                printf("%s", EOL);

                printf("%s" CLR_KEY "?" OFF " Press any key to start this mode...%s", UI_INDENT, EOL);
                char cmd = getc(stdin);
                printf(BOLD);
                printf("%s" CLR_OK "[+]" OFF " Mode started. The USB device will now reconnect.%s%s", UI_INDENT, EOL, EOL);
                sleep_ms(4);
                printf(OFF);
                fflush(stdout);
                busyLoop(USB_HOST_RECOGNISE_TIME);
                tud_disconnect();
                multicore_reset_core1(); 
                stdio_set_driver_enabled(&stdio_usb, false);
                stdio_deinit_all();                
                sleep_us(50);
                usbMode = USB_MODE_CMSISDAP;
                initUART();
                cmsisDapInit();
                break;

            default:
                printf("%s" CLR_FAIL "[-]" OFF " Unknown command '%c'. Press 'h' for help.%s%s", UI_INDENT, cmd, EOL, EOL);
                break;
        }
        showPrompt();
    }    
    return 0;
}
