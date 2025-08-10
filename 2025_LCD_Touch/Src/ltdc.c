/*
 * ltdc.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <usart.h>
#include <mem_io.h>
#include <ltdc.h>

void ltdc_gpio_init(void)
{
    // Enable GPIO clocks
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOB);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

    // RED pins (R0~R5)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // R0
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, 10, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // R1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_0, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 0, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // R2
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, 11, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // R3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_12, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, 12, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // R4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_1, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 1, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // R5
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, 6, GPIO_SPEED_VERY_HIGH);

    // GREEN pins (G0~G5)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // G0
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, 6, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, 10, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G2
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 10, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // G3
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 11, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // G4
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_7, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, 7, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOD_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // G5
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_3, ALTERNATE_AF14);
    gpio_set_speed(GPIOD_BASE, 3, GPIO_SPEED_VERY_HIGH);

    // BLUE pins (B0~B5)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // B0
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOD_BASE, 6, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // B1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, 11, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // B2
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_12, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, 12, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // B3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_3, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, 3, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // B4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_8, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 8, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // B5
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_9, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, 9, GPIO_SPEED_VERY_HIGH);

    // LCD control signals
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // DE
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOF_BASE, 10, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // DOTCLK
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_7, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, 7, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // HSYNC
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, 6, GPIO_SPEED_VERY_HIGH);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);  // VSYNC
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_4, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, 4, GPIO_SPEED_VERY_HIGH);
}

void fill_framebuffer(uint16_t color) {
    uint16_t *fb = (uint16_t*)FRAMEBUFFER_ADDR;
    for (int i = 0; i < 240 * 320; i++) {
        fb[i] = color; // 0xF800 for red (RGB565)
    }
}

void ltdc_init(void){
	// Step 0: Init GPIOs for LTDC signals (RGB, HSYNC, VSYNC, DE, CLK)
	ltdc_gpio_init();



	fill_framebuffer(0xF800);


	// Step 1: Enable LTDC clock on APB2
	rcc_enable_apb2_clock(RCC_APB2EN_LTDC);

	// Step 2: Configure PLLSAI for LCD pixel clock
	// VCO = HSI(16 MHz) / 8 * 128 = 256 MHz
	// LCD clock = 256 / 4 / 8 = 8 MHz
	rcc_pllcfgr_write_field(LTDC_PLLCFGR_PLLSRC, 0);             // HSI selected
    rcc_pllcfgr_write_field(LTDC_PLLCFGR_PLLM,   8);             // PLL input clock / 8
    rcc_pllsaicfgr_write_field(LTDC_PLLSAICFGR_PLLSAIN, 128);    // PLLSAI VCO multiplier
    rcc_pllsaicfgr_write_field(LTDC_PLLSAICFGR_PLLSAIR, 4);      // PLLSAI /R division
    rcc_dckcfgr_write_field(LTDC_DCKCFGR_PLLSAIDIVR, 0b10);      // DIVR = /8 for final LCD clock
    rcc_cr_write_field(LTDC_CR_PLLSAION, 1);                     // Enable PLLSAI
	delay_us(10000);
	while (!rcc_cr_read_field(LTDC_CR_PLLSAIRDY)) { /* wait */ }
	usart_printf("PLLSAI = %d\r\n",rcc_cr_read_field(LTDC_CR_PLLSAIRDY));

    // Step 3: Configure LTDC timing registers
	ltdc_sscr_set_field(LTDC_SSCR_FIELD_HSW, LTDC_SSCR_HSW);     // HSYNC width
	ltdc_sscr_set_field(LTDC_SSCR_FIELD_VSH, LTDC_SSCR_VSH);     // VSYNC height
	ltdc_bpcr_set_field(LTDC_BPCR_FIELD_AHBP, LTDC_BPCR_AHBP);   // Horizontal back porch
	ltdc_bpcr_set_field(LTDC_BPCR_FIELD_AVBP, LTDC_BPCR_AVBP);   // Vertical back porch
	ltdc_awcr_set_field(LTDC_AWCR_FIELD_AAW, LTDC_AWCR_AAW);     // Active width
	ltdc_awcr_set_field(LTDC_AWCR_FIELD_AAH, LTDC_AWCR_AAH);     // Active height
	ltdc_twcr_set_field(LTDC_TWCR_FIELD_TOTALW, LTDC_TWCR_TOTALW); // Total width
	ltdc_twcr_set_field(LTDC_TWCR_FIELD_TOTALH, LTDC_TWCR_TOTALH); // Total height

    // Step 4: Configure polarity settings in LTDC_GCR
    //         Match the ILI9341 SPI initialization parameters:
    //         - VSPL = 1 → VSYNC is active high   → VSPOL = 1
    //         - HSPL = 1 → HSYNC is active high   → HSPOL = 1
    //         - EPL  = 0 → DE is active high      → DEPOL = 1
    //         - DPL  = 0 → Data on DOTCLK rising  → PCPOL = 0
    ltdc_gcr_set_field(LTDC_GCR_FIELD_DEPOL, 1);   // DE polarity: active high
    ltdc_gcr_set_field(LTDC_GCR_FIELD_VSPOL, 1);   // VSYNC polarity: active high
    ltdc_gcr_set_field(LTDC_GCR_FIELD_HSPOL, 1);   // HSYNC polarity: active high
    ltdc_gcr_set_field(LTDC_GCR_FIELD_PCPOL, 0);   // Pixel clock polarity: normal (rising edge latch)

    // Step 5: Set background color
    ltdc_bccr_set_rgb565_color(255, 0, 0);  // Red background

    // Step 6: (Optional) Setup interrupts

    // Step 7: Configure Layer 1 (foreground)
    // 7.1 Set Layer 1 window position within the active display area (defined by AWCR)
    ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSTPOS, LTDC_LAYER_WHSTPOS);  // Horizontal start = AHBP + 1 (first visible pixel)
    ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSPPOS, LTDC_LAYER_WHSPPOS);  // Horizontal stop  = AAW (last visible pixel)
    ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSTPOS, LTDC_LAYER_WVSTPOS);  // Vertical start = AVBP + 1 (first visible line)
    ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSPPOS, LTDC_LAYER_WVSPPOS);  // Vertical stop  = AAH (last visible line)

    // 7.2 Set pixel format for Layer 1 (RGB565 = 16-bit color)
    ltdc_lxpfcr_set_field(1, LTDC_PIXEL_FORMAT_RGB565);

    // 7.3 Set frame buffer start address for Layer 1
    ltdc_lxcfbar_set_field(1, FRAMEBUFFER_ADDR);

    // 7.4 Set frame buffer line length and pitch (in bytes)
    ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBLL, LTDC_ACTIVE_WIDTH); // Line length = width * bytes per pixel + 3
    ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBP, LTDC_ACTIVE_WIDTH);  // Pitch = width * bytes per pixel

    // 7.5 Set number of active display lines (height)
    ltdc_lxcfblnr_set_field(1, LTDC_ACTIVE_HEIGHT);

    // Step 8: Enable Layer 1, disable Layer 2
    //ltdc_lxcr_set_field(1, LTDC_LXCR_FIELD_LEN, 1);  // Enable Layer 1
    ltdc_lxcr_set_field(1, LTDC_LXCR_FIELD_LEN, 1);  // Disable Layer 1
    ltdc_lxcr_set_field(2, LTDC_LXCR_FIELD_LEN, 0);  // Disable Layer 2

    // Step 9: Reload shadow registers immediately
    ltdc_srcr_set_field(LTDC_SRCR_FIELD_IMR, 1);

    // Step 10: Enable LTDC controller (LTDC_GCR bit 0 = 1)
    ltdc_gcr_set_field(LTDC_GCR_FIELD_LTDCEN, 1);
}

void ltdc_sscr_set_field(ltdc_sscr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_SSCR_OFFSET;
    uint32_t shift = 0, width = 0;

	switch (field) {
		case LTDC_SSCR_FIELD_VSH: shift = 0; width = 11; break;
		case LTDC_SSCR_FIELD_HSW: shift = 16; width = 12; break;
		default: return;
	}

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_bpcr_set_field(ltdc_bpcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_BPCR_OFFSET;
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_BPCR_FIELD_AVBP: shift = 0;  width = 11; break;
        case LTDC_BPCR_FIELD_AHBP: shift = 16; width = 12; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_awcr_set_field(ltdc_awcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_AWCR_OFFSET;
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_AWCR_FIELD_AAH: shift = 0;  width = 11; break;
        case LTDC_AWCR_FIELD_AAW: shift = 16; width = 12; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_twcr_set_field(ltdc_twcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_TWCR_OFFSET;
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_TWCR_FIELD_TOTALH: shift = 0;  width = 11; break;
        case LTDC_TWCR_FIELD_TOTALW: shift = 16; width = 12; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_gcr_set_field(ltdc_gcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_GCR_OFFSET;
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_GCR_FIELD_LTDCEN: shift = 0;  width = 1; break;
        case LTDC_GCR_FIELD_DBW:    shift = 4;  width = 3; break;  // 6:4
        case LTDC_GCR_FIELD_DGW:    shift = 8;  width = 3; break;  // 10:8
        case LTDC_GCR_FIELD_DRW:    shift = 12; width = 3; break;  // 14:12
        case LTDC_GCR_FIELD_DEN:    shift = 16; width = 1; break;
        case LTDC_GCR_FIELD_PCPOL:  shift = 28; width = 1; break;
        case LTDC_GCR_FIELD_DEPOL:  shift = 29; width = 1; break;
        case LTDC_GCR_FIELD_VSPOL:  shift = 30; width = 1; break;
        case LTDC_GCR_FIELD_HSPOL:  shift = 31; width = 1; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;

    io_writeMask(addr, data, mask);
}

uint8_t to_rgb565_r(uint8_t r8) {
    return r8 >> 3; // R: 8-bit to 5-bit
}

uint8_t to_rgb565_g(uint8_t g8) {
    return g8 >> 2; // G: 8-bit to 6-bit
}

uint8_t to_rgb565_b(uint8_t b8) {
    return b8 >> 3; // B: 8-bit to 5-bit
}

void ltdc_bccr_set_rgb565_color(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t addr = LTDC_BASE + LTDC_BCCR_OFFSET;

    uint32_t r = to_rgb565_r(red) << 3;
    uint32_t g = to_rgb565_g(green) << 2;
    uint32_t b = to_rgb565_b(blue) << 3;

    uint32_t data = (r << 16) | (g << 8) | b;
    uint32_t mask = 0x00FFFFFFU;

    io_writeMask(addr, data, mask);
}

void ltdc_lxwhpcr_set_field(uint8_t layerx, ltdc_lxwhpcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_LXWHPCR_OFFSET(layerx);
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_LXWHPCR_FIELD_WHSTPOS: shift = 0;  width = 12; break;
        case LTDC_LXWHPCR_FIELD_WHSPPOS: shift = 16; width = 12; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_lxwvpcr_set_field(uint8_t layerx, ltdc_lxwvpcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_LXWVPCR_OFFSET(layerx);
    uint32_t shift = 0, width = 0;

    switch (field) {
        case LTDC_LXWVPCR_FIELD_WVSTPOS: shift = 0;  width = 12; break;
        case LTDC_LXWVPCR_FIELD_WVSPPOS: shift = 16; width = 12; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_lxpfcr_set_field(uint8_t layerx, uint32_t pixel_format) {
    uint32_t addr = LTDC_BASE + LTDC_LXPFCR_OFFSET(layerx);
    uint32_t mask = 0x07U;
    uint32_t data = pixel_format & mask;
    io_writeMask(addr, data, mask);
}

void ltdc_lxcfbar_set_field(uint8_t layerx, uint32_t ram_addr) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBAR_OFFSET(layerx);
    io_writeMask(addr, ram_addr, 0xFFFFFFFF);
}

void ltdc_lxcfblr_set_field(uint8_t layerx, ltdc_lxcfblr_field_t field, uint32_t lcd_width) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLR_OFFSET(layerx);
    uint32_t shift = 0, data = 0;

    switch (field) {
        case LTDC_LXCFBLR_FIELD_CFBLL:
            shift = 0;
            data = (lcd_width * 2 + 3) << shift;
            break;
        case LTDC_LXCFBLR_FIELD_CFBP:
            shift = 16;
            data = (lcd_width * 2) << shift;
            break;
        default: return;
    }

    uint32_t mask = 0x1FFF << shift;
    io_writeMask(addr, data, mask);
}

void ltdc_lxcfblnr_set_field(uint8_t layerx, uint32_t height) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLNR_OFFSET(layerx);
    uint32_t mask = 0x7FF;
    io_writeMask(addr, height, mask);
}

void ltdc_lxcr_set_field(uint8_t layerx, ltdc_lxcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_LXCR_OFFSET(layerx);
    uint32_t shift = 0;

    switch (field) {
    	case LTDC_LXCR_FIELD_LEN: shift = 0; break;
    	case LTDC_LXCR_FIELD_COLKEN: shift = 1; break;
    	case LTDC_LXCR_FIELD_CLUTEN: shift = 4; break;
    	default: return;
    }

    uint32_t mask = 1U << shift;
    uint32_t data = value << shift;

    io_writeMask(addr, data, mask);
}

void ltdc_srcr_set_field(ltdc_srcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_SRCR_OFFSET;
    uint32_t mask = 1U << field;
    uint32_t data = value << field;

    io_writeMask(addr, data, mask);
}


