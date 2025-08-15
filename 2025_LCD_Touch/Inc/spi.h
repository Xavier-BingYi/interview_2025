/*
 * spi.h
 *
 *  Created on: Jul 25, 2025
 *      Author: Xavier
 */

#ifndef SPI_H_
#define SPI_H_

typedef enum {
    SPI_CR1_CPHA = 0,  // Clock Phase
    SPI_CR1_CPOL,      // Clock Polarity
    SPI_CR1_MSTR,      // Master Selection
    SPI_CR1_BR,        // Baud Rate control (3 bits: BR[2:0] starts from bit 3)
    SPI_CR1_SPE,       // SPI Enable
    SPI_CR1_LSBFIRST,  // Frame Format (LSB/MSB first)
    SPI_CR1_SSI,       // Internal Slave Select
    SPI_CR1_SSM,       // Software Slave Management
    SPI_CR1_RXONLY,    // Receive Only
    SPI_CR1_DFF,       // Data Frame Format (8/16 bit)
    SPI_CR1_CRCNEXT,   // CRC Transfer Next
    SPI_CR1_CRCEN,     // Hardware CRC Enable
    SPI_CR1_BIDIOE,    // Output Enable in Bidirectional Mode
    SPI_CR1_BIDIMODE   // Bidirectional Data Mode Enable
} spi_cr1_field_t;

typedef enum {
    SPI_CR2_RXDMAEN = 0,  // Rx buffer DMA enable
    SPI_CR2_TXDMAEN = 1, // Tx buffer DMA enable
    SPI_CR2_SSOE = 2,    // SS output enable
    // Bit3 reserved, no enum
    SPI_CR2_FRF = 4,     // Frame format
    SPI_CR2_ERRIE = 5,       // Error interrupt enable
    SPI_CR2_RXNEIE = 6,      // RX buffer not empty interrupt enable
    SPI_CR2_TXEIE = 7   // TX buffer empty interrupt enable
} spi_cr2_field_t;

typedef enum {
    SPI_SR_RXNE     = 0,  // Receive buffer Not Empty
    SPI_SR_TXE      = 1,  // Transmit buffer Empty
    SPI_SR_CHSIDE   = 2,  // Channel side (only relevant in I2S mode)
    SPI_SR_UDR      = 3,  // Underrun flag (I2S mode only)
    SPI_SR_CRCERR   = 4,  // CRC Error flag (clear by writing 0)
    SPI_SR_MODF     = 5,  // Mode Fault
    SPI_SR_OVR      = 6,  // Overrun flag
    SPI_SR_BSY      = 7,  // Busy flag
    SPI_SR_FRE      = 8   // Frame Format Error
} spi_sr_field_t;

void spi_gpio_init(void);
void spi_init(void);
void spi_cr1_write_field(spi_cr1_field_t field, uint32_t value);
void spi_cr2_write_field(spi_cr2_field_t field, uint32_t value);
uint8_t spi_sr_read_field(spi_sr_field_t field);
void spi_lcd_write_command(uint8_t data);
void spi_lcd_write_data(uint8_t data);
void spi_lcd_send_end(void);
void ili9341_init(void);

#endif /* SPI_H_ */
