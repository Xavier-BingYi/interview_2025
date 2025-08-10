/*
 * spi.c
 *
 *  Created on: Jul 25, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>
#include <gpio.h>
#include <rcc.h>
#include <spi.h>
#include <ltdc.h>

void spi_gpio_init(void)
{
    // Enable AHB1 clocks for GPIOF, GPIOD, and GPIOC
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);  // for SCK, MOSI
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);  // for D/C (WRX)
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);  // for CS

    // Set PF9 to SPI5_MOSI (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_9, ALTERNATE_AF5);
    gpio_set_speed(GPIOF_BASE, 9, GPIO_SPEED_VERY_HIGH);

    // Set PF7 to SPI5_SCK (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_7, ALTERNATE_AF5);
    gpio_set_speed(GPIOF_BASE, 7, GPIO_SPEED_VERY_HIGH);

    // Set PD13 as output for D/C (Data/Command select)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_13, GPIO_MODE_OUTPUT);

    // Set PC2 as output for CS (Chip Select)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_2, GPIO_MODE_OUTPUT);
}

void spi_init(void) {
    // Enable APB2 clock for SPI5 peripheral
    rcc_enable_apb2_clock(RCC_APB2EN_SPI5);

    // Initialize GPIO pins for SPI5 (SCK, MOSI, CS, D/C)
    spi_gpio_init();

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 0); // CS = 0, always selected
    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 0); // D/C = 0, always command

    // According to ILI9341 spec, SPI max clock = 10 MHz
    // Set SPI baud rate to fPCLK / 2 = 8 MHz (assuming default fPCLK = 16 MHz)
    spi_cr1_write_field(SPI_CR1_BR, 0b000);  // 0b000: baud rate divisor = 2

    // CPOL: CK=0 when idle; CPHA: capture on first clock edge
    spi_cr1_write_field(SPI_CR1_CPOL, 0);
    spi_cr1_write_field(SPI_CR1_CPHA, 0);

    // Set data frame format to 8-bit (DFF=0)
    spi_cr1_write_field(SPI_CR1_DFF, 0);

    // Use MSB-first format (LSBFIRST = 0, default)
    spi_cr1_write_field(SPI_CR1_LSBFIRST, 0);

    // Enable software slave management (SSM = 1)
    // Simulate NSS as not selected (SSI = 1)
    spi_cr1_write_field(SPI_CR1_SSM, 1);
    spi_cr1_write_field(SPI_CR1_SSI, 1);

    // Configure as SPI master (MSTR = 1)
    // Enable SPI peripheral (SPE = 1)
    spi_cr1_write_field(SPI_CR1_MSTR, 1);
    spi_cr1_write_field(SPI_CR1_SPE, 1);

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 1);  // CS = 1 (idle high)
}

void spi_cr1_write_field(spi_cr1_field_t field, uint32_t value){
    uint32_t addr = SPI_BASE + SPI_CR1_OFFSET;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case SPI_CR1_CPHA:         shift = 0;  width = 1; break;
        case SPI_CR1_CPOL:         shift = 1;  width = 1; break;
        case SPI_CR1_MSTR:         shift = 2;  width = 1; break;
        case SPI_CR1_BR:           shift = 3;  width = 3; break;
        case SPI_CR1_SPE:          shift = 6;  width = 1; break;
        case SPI_CR1_LSBFIRST:     shift = 7;  width = 1; break;
        case SPI_CR1_SSI:          shift = 8;  width = 1; break;
        case SPI_CR1_SSM:          shift = 9;  width = 1; break;
        case SPI_CR1_RXONLY:       shift = 10; width = 1; break;
        case SPI_CR1_DFF:          shift = 11; width = 1; break;
        case SPI_CR1_CRCNEXT:      shift = 12; width = 1; break;
        case SPI_CR1_CRCEN:        shift = 13; width = 1; break;
        case SPI_CR1_BIDIOE:       shift = 14; width = 1; break;
        case SPI_CR1_BIDIMODE:     shift = 15; width = 1; break;
        default: return;
    }
    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

uint8_t spi_sr_read_field(spi_sr_field_t field) {
    uint32_t addr = SPI_BASE + SPI_SR_OFFSET;
    return (io_read(addr) & (1U << (uint32_t)field)) ? 1 : 0;
}

void spi_lcd_write_command(uint32_t data) {
    uint32_t addr = SPI_BASE + SPI_DR_OFFSET;

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 0); // CS low
    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 0); // D/C = 0 (command)

    while (spi_sr_read_field(SPI_SR_TXE) == 0);
    io_write(addr, data);
    while (spi_sr_read_field(SPI_SR_BSY) == 1);

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 1); // CS high
}

void spi_lcd_write_data(uint32_t data) {
    uint32_t addr = SPI_BASE + SPI_DR_OFFSET;

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 0); // CS low
    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 1); // D/C = 1 (data)

    while (spi_sr_read_field(SPI_SR_TXE) == 0);
    io_write(addr, data);
    while (spi_sr_read_field(SPI_SR_BSY) == 1);

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 1); // CS high
}

void ili9341_init(void){
	// Step 1: Software Reset
	spi_lcd_write_command(0x01); // 0x01 = Software Reset command (D/CX = 0)
	delay_us(5000);              // Wait ≥ 5ms as required by ILI9341 datasheet

	// Step 2: Exit Sleep Mode
	spi_lcd_write_command(0x11);   // 0x11 = Sleep Out command (D/CX = 0)
	delay_us(120000);              // Wait ≥120ms to allow power/clocks to stabilize




	// Power Control 1
	spi_lcd_write_command(0xC0);
	spi_lcd_write_data(0x26);

	// Power Control 2
	spi_lcd_write_command(0xC1);
	spi_lcd_write_data(0x11);

	// VCOM Control 1
	spi_lcd_write_command(0xC5);
	spi_lcd_write_data(0x31);
	spi_lcd_write_data(0x3C);

	// VCOM Control 2
	spi_lcd_write_command(0xC7);
	spi_lcd_write_data(0xB0);





	// Step 3: Configure RGB interface signal mode
	spi_lcd_write_command(0xB0);    // RGB Interface Signal Control
	spi_lcd_write_data(0x4C);       // DE mode, DOTCLK rising

	// Step 4: Set pixel format to 16-bit RGB (DPI[2:0] = 101)
	spi_lcd_write_command(0x3A);     // 0x3A = Pixel Format Set
	spi_lcd_write_data(0x55);        // 0x55 = 16-bit RGB (DPI[2:0] = 101, DBI ignored)

	// Step 5: Interface Control
	spi_lcd_write_command(0xF6);   // Interface Control
	spi_lcd_write_data(0x01);      // Data 1: WEMODE=1 (avoid overflow), others default
	spi_lcd_write_data(0x00);      // Data 2: MDT=00, EPF=00 (default, unused in RGB)
	spi_lcd_write_data(0x06);      // Data 3: DM=01 (RGB mode)、RM=1、RIM=0

	// Step 6: Set Memory Access Control (scan direction, RGB/BGR order)
	spi_lcd_write_command(0x36);     // 0x36 = Memory Access Control
	spi_lcd_write_data(0x00);        // MY=0, MX=0, MV=0, ML=0, BGR=0, MH=0








	// Positive Gamma Correction
	spi_lcd_write_command(0xE0);
	spi_lcd_write_data(0x0F);
	spi_lcd_write_data(0x31);
	spi_lcd_write_data(0x2B);
	spi_lcd_write_data(0x0C);
	spi_lcd_write_data(0x0E);
	spi_lcd_write_data(0x08);
	spi_lcd_write_data(0x4E);
	spi_lcd_write_data(0xF1);
	spi_lcd_write_data(0x37);
	spi_lcd_write_data(0x07);
	spi_lcd_write_data(0x10);
	spi_lcd_write_data(0x03);
	spi_lcd_write_data(0x0E);
	spi_lcd_write_data(0x09);
	spi_lcd_write_data(0x00);

	// Negative Gamma Correction
	spi_lcd_write_command(0xE1);
	spi_lcd_write_data(0x00);
	spi_lcd_write_data(0x0E);
	spi_lcd_write_data(0x14);
	spi_lcd_write_data(0x03);
	spi_lcd_write_data(0x11);
	spi_lcd_write_data(0x07);
	spi_lcd_write_data(0x31);
	spi_lcd_write_data(0xC1);
	spi_lcd_write_data(0x48);
	spi_lcd_write_data(0x08);
	spi_lcd_write_data(0x0F);
	spi_lcd_write_data(0x0C);
	spi_lcd_write_data(0x31);
	spi_lcd_write_data(0x36);
	spi_lcd_write_data(0x0F);

	// Display Function Control
	spi_lcd_write_command(0xB6);
	spi_lcd_write_data(0x0A);
	spi_lcd_write_data(0x82);
	spi_lcd_write_data(0x27);








	// Step 7: Normal Display Mode ON
	spi_lcd_write_command(0x13);     // 0x13 = Normal Display Mode ON
	delay_us(10000);

	// Step 8: Turn on display
	spi_lcd_write_command(0x29);     // 0x29 = Display ON
	delay_us(10000);
}
