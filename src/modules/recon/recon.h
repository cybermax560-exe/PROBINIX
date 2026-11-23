#ifndef PROBINIX_RECON_H
#define PROBINIX_RECON_H

// Target reconnaissance tools. Each one returns to the PROBINIX menu when done.
void reconPinMonitor(void);     // Report state and activity of each channel
void reconUartScan(void);       // Passively find UART TX pins and baud rates
void reconI2cScan(void);        // Find devices on an I2C bus
void reconSpiFlashId(void);     // Read the JEDEC ID of an SPI flash chip

#endif
