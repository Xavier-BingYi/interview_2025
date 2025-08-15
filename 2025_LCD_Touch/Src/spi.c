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
    // Enable AHB1 clocks for GPIOF, GPIOD, GPIOC, and GPIOA
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);  // for SCK, MOSI
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);  // for D/C (WRX)
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);  // for CS
	rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);  // for RESX

    // Set PF9 to SPI5_MOSI (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_9, ALTERNATE_AF5);
    gpio_set_output_type(GPIOF_BASE, GPIO_PIN_9, GPIO_OTYPE_PUSHPULL);
    gpio_set_speed(GPIOF_BASE, GPIO_PIN_9, GPIO_SPEED_HIGH);

    // Set PF7 to SPI5_SCK (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_7, ALTERNATE_AF5);
    gpio_set_output_type(GPIOF_BASE, GPIO_PIN_7, GPIO_OTYPE_PUSHPULL);
    gpio_set_speed(GPIOF_BASE, GPIO_PIN_7, GPIO_SPEED_HIGH);

    // Set PD13 as output for D/C (Data/Command select)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_13, GPIO_MODE_OUTPUT);
    gpio_set_output_type(GPIOD_BASE, GPIO_PIN_13, GPIO_OTYPE_PUSHPULL);
    gpio_set_speed(GPIOD_BASE, GPIO_PIN_13, GPIO_SPEED_HIGH);

    // Set PC2 as output for CS (Chip Select)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_2, GPIO_OTYPE_PUSHPULL);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_2, GPIO_SPEED_HIGH);

    // Set PA10 as output for RESX
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_10, GPIO_MODE_OUTPUT);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_10, GPIO_OTYPE_PUSHPULL);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_10, GPIO_SPEED_HIGH);


    // Safe idle levels BEFORE touching SPI: CS=1 (inactive), D/C=0 (command)
    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 0); // D/C = command
    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 1);  // CS idle high
    gpio_set_outdata(GPIOA_BASE, GPIO_PIN_10, 1);
}

void spi_init(void) {
    // Initialize GPIO pins for SPI5 (SCK, MOSI, CS, D/C)
    spi_gpio_init();

    // Enable APB2 clock for SPI5 peripheral
    rcc_enable_apb2_clock(RCC_APB2EN_SPI5);

    // Disable SPI peripheral (SPE = 0)
    spi_cr1_write_field(SPI_CR1_SPE, 0);

    // According to ILI9341 spec, SPI max clock = 10 MHz
    // Set SPI baud rate to fPCLK / 16 = 5.625 MHz (assuming fPCLK = 90 MHz)
    spi_cr1_write_field(SPI_CR1_BR, 0b011);  // 0b011: baud rate divisor = 16

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

    // 1-line & Tx-only
    spi_cr1_write_field(SPI_CR1_BIDIMODE, 1);
    spi_cr1_write_field(SPI_CR1_BIDIOE, 1);

    // Configure as SPI master (MSTR = 1)
    spi_cr1_write_field(SPI_CR1_MSTR, 1);

    // Enable SPI peripheral (SPE = 1)
    spi_cr1_write_field(SPI_CR1_SPE, 1);
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

void spi_cr2_write_field(spi_cr2_field_t field, uint32_t value){
    uint32_t addr = SPI_BASE + SPI_CR2_OFFSET;
    uint8_t shift = 0;
    uint8_t width = 1;

    switch (field) {
        case SPI_CR2_TXEIE:    shift = 7; break;
        case SPI_CR2_RXNEIE:   shift = 6; break;
        case SPI_CR2_ERRIE:    shift = 5; break;
        case SPI_CR2_FRF:      shift = 4; break;
        case SPI_CR2_SSOE:     shift = 2; break;
        case SPI_CR2_TXDMAEN:  shift = 1; break;
        case SPI_CR2_RXDMAEN:  shift = 0; break;
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

void spi_lcd_write_command(uint8_t data) {
    uint32_t addr = SPI_BASE + SPI_DR_OFFSET;

    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 0); // CS low
    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 0); // D/C = 0 (command)

    while (spi_sr_read_field(SPI_SR_TXE) == 0);
    io_write8(addr, data);
    while (spi_sr_read_field(SPI_SR_BSY) == 1);
}

void spi_lcd_write_data(uint8_t data) {
    uint32_t addr = SPI_BASE + SPI_DR_OFFSET;

    gpio_set_outdata(GPIOD_BASE, GPIO_PIN_13, 1); // D/C = 1 (data)

    while (spi_sr_read_field(SPI_SR_TXE) == 0);
    io_write8(addr, data);
    while (spi_sr_read_field(SPI_SR_BSY) == 1);
}

void spi_lcd_send_end(void) {
    while (spi_sr_read_field(SPI_SR_BSY));
    gpio_set_outdata(GPIOC_BASE, GPIO_PIN_2, 1); // CS High
}

void ili9341_init(void){
	// Step 1: Software Reset
	spi_lcd_write_command(0x01); // 0x01 = Software Reset command (D/CX = 0)
	spi_lcd_send_end();
	delay_us(5000);              // Wait ≥ 5ms as required by ILI9341 datasheet

	// Step 2: Exit Sleep Mode
	spi_lcd_write_command(0x11);   // 0x11 = Sleep Out command (D/CX = 0)
	spi_lcd_send_end();
	delay_us(120000);              // Wait ≥120ms to allow power/clocks to stabilize

	// Step 3: Configure RGB interface signal mode (ILI9341 reg 0xB0)
	// 0xC0 breakdown:
	//   bit7 ByPass_MODE=1  -> Memory path
	//   bit6..5 RCM=10      -> RGB interface mode
	//   bit4 = 0            -> reserved
	//   bit3 VSPL=0         -> VSYNC active low
	//   bit2 HSPL=0         -> HSYNC active low
	//   bit1 DPL=0          -> sample on rising DOTCLK
	//   bit0 EPL=0          -> DE active high
	spi_lcd_write_command(0xB0);    // RGB Interface Signal Control
	spi_lcd_write_data(0xC0);
	spi_lcd_send_end();

	// Step 4: Set pixel format to 16-bit RGB (DPI[2:0] = 101)
	spi_lcd_write_command(0x3A);     // 0x3A = Pixel Format Set
	spi_lcd_write_data(0x55);        // 0x55 = 16-bit RGB (DPI[2:0] = 101, DBI ignored)
	spi_lcd_send_end();

	// Step 5: Interface Control
	spi_lcd_write_command(0xF6);   // Interface Control
	spi_lcd_write_data(0x00);      // Data 1: WEMODE=1 (avoid overflow), others default
	spi_lcd_write_data(0x00);      // Data 2: MDT=00, EPF=00 (default, unused in RGB)
	spi_lcd_write_data(0x06);      // Data 3: DM=01 (RGB mode, not DE mode)、RM=1、RIM=0
	spi_lcd_send_end();

	// Step 6: Set Memory Access Control (scan direction, RGB/BGR order)
	spi_lcd_write_command(0x36);     // 0x36 = Memory Access Control
	spi_lcd_write_data(0xC8);
	spi_lcd_send_end();

	// Step 7: Normal Display Mode ON
	spi_lcd_write_command(0x13);     // 0x13 = Normal Display Mode ON
	spi_lcd_send_end();
	delay_us(10000);

	// Step 8: Turn on display
	spi_lcd_write_command(0x29);     // 0x29 = Display ON
	spi_lcd_send_end();
	delay_us(10000);






	// Positive Gamma Correction
	spi_lcd_write_command(0xE0);   // ILI9341_PGAMMA
	spi_lcd_write_data(0x0F);
	spi_lcd_write_data(0x1D);
	spi_lcd_write_data(0x1A);
	spi_lcd_write_data(0x0A);
	spi_lcd_write_data(0x0D);
	spi_lcd_write_data(0x07);
	spi_lcd_write_data(0x49);
	spi_lcd_write_data(0x66);
	spi_lcd_write_data(0x3B);
	spi_lcd_write_data(0x07);
	spi_lcd_write_data(0x11);
	spi_lcd_write_data(0x01);
	spi_lcd_write_data(0x09);
	spi_lcd_write_data(0x05);
	spi_lcd_write_data(0x04);
	spi_lcd_send_end();

	// Negative Gamma Correction
	spi_lcd_write_command(0xE1);   // ILI9341_NGAMMA
	spi_lcd_write_data(0x00);
	spi_lcd_write_data(0x18);
	spi_lcd_write_data(0x1D);
	spi_lcd_write_data(0x02);
	spi_lcd_write_data(0x0F);
	spi_lcd_write_data(0x04);
	spi_lcd_write_data(0x36);
	spi_lcd_write_data(0x13);
	spi_lcd_write_data(0x4C);
	spi_lcd_write_data(0x07);
	spi_lcd_write_data(0x13);
	spi_lcd_write_data(0x0F);
	spi_lcd_write_data(0x2E);
	spi_lcd_write_data(0x2F);
	spi_lcd_write_data(0x05);
	spi_lcd_send_end();

	// Display Function Control
	spi_lcd_write_command(0xB6);
	spi_lcd_write_data(0x0A);
	spi_lcd_write_data(0xA2);
	spi_lcd_send_end();

	// Power Control 1
	spi_lcd_write_command(0xC0);
	spi_lcd_write_data(0x1B);
	spi_lcd_send_end();

	// Power Control 2
	spi_lcd_write_command(0xC1);
	spi_lcd_write_data(0x12);
	spi_lcd_send_end();

	// VCOM Control 1
	spi_lcd_write_command(0xC5);
	spi_lcd_write_data(0x08);
	spi_lcd_write_data(0x26);
	spi_lcd_send_end();

	// VCOM Control 2
	spi_lcd_write_command(0xC7);
	spi_lcd_write_data(0XB7);
	spi_lcd_send_end();

    // RASET: rows 0..319 (0x01,0x3F)
    spi_lcd_write_command(0x2B);
    spi_lcd_write_data(0x00);
    spi_lcd_write_data(0x00);
    spi_lcd_write_data(0x01);
    spi_lcd_write_data(0x3F);
    spi_lcd_send_end();

    // CASET: cols 0..239 (0x00,0xEF)
    spi_lcd_write_command(0x2A);
    spi_lcd_write_data(0x00);
    spi_lcd_write_data(0x00);
    spi_lcd_write_data(0x00);
    spi_lcd_write_data(0xEF);
    spi_lcd_send_end();
}
