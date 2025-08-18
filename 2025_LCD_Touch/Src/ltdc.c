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

    // RED pins (R2~R7)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // R2
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_10, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_10, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // R3
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_0, ALTERNATE_AF9);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_0, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_0, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // R4
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_11, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_11, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // R5
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_12, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_12, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_12, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // R6
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_1, ALTERNATE_AF9);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_1, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_1, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // R7
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, GPIO_PIN_6, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOG_BASE, GPIO_PIN_6, GPIO_OTYPE_PUSHPULL);

    // GREEN pins (G2~G7)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // G2
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_6, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_6, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G3
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_10, ALTERNATE_AF9);
    gpio_set_speed(GPIOG_BASE, GPIO_PIN_10, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOG_BASE, GPIO_PIN_10, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_10, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_10, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // G5
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_11, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_11, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // G6
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_7, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_7, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_7, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOD_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // G7
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_3, ALTERNATE_AF14);
    gpio_set_speed(GPIOD_BASE, GPIO_PIN_3, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOD_BASE, GPIO_PIN_3, GPIO_OTYPE_PUSHPULL);

    // BLUE pins (B2~B7)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // B2
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOD_BASE, GPIO_PIN_6, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOD_BASE, GPIO_PIN_6, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // B3
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_11, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, GPIO_PIN_11, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOG_BASE, GPIO_PIN_11, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // B4
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_12, ALTERNATE_AF9);
    gpio_set_speed(GPIOG_BASE, GPIO_PIN_12, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOG_BASE, GPIO_PIN_12, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // B5
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_3, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_3, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_3, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // B6
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_8, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_8, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_8, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // B7
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_9, ALTERNATE_AF14);
    gpio_set_speed(GPIOB_BASE, GPIO_PIN_9, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOB_BASE, GPIO_PIN_9, GPIO_OTYPE_PUSHPULL);

    // LCD control signals
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // DE
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_10, ALTERNATE_AF14);
    gpio_set_speed(GPIOF_BASE, GPIO_PIN_10, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOF_BASE, GPIO_PIN_10, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // DOTCLK
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_7, ALTERNATE_AF14);
    gpio_set_speed(GPIOG_BASE, GPIO_PIN_7, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOG_BASE, GPIO_PIN_7, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // HSYNC
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_6, ALTERNATE_AF14);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_6, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_6, GPIO_OTYPE_PUSHPULL);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);  // VSYNC
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_4, ALTERNATE_AF14);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_4, GPIO_SPEED_HIGH);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_4, GPIO_OTYPE_PUSHPULL);
}


uint16_t Convert_RGB888_to_RGB565(uint32_t rgb888){
    uint8_t r = (rgb888 >> 16) & 0xFF;
    uint8_t g = (rgb888 >>  8) & 0xFF;
    uint8_t b = (rgb888 >>  0) & 0xFF;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void write_to_fb_rgb565(uint16_t *fb_ptr, uint32_t n_pixels, uint16_t rgb565){
    while (n_pixels--) {
        *fb_ptr++ = rgb565;
    }
}

void fill_framebuffer(uint16_t color){
    write_to_fb_rgb565((uint16_t *)FRAMEBUFFER_ADDR, LTDC_ACTIVE_WIDTH * LTDC_ACTIVE_HEIGHT, color);
}

void fill_framebuffer_rgb888(uint32_t rgb888)
{
    fill_framebuffer(Convert_RGB888_to_RGB565(rgb888));
}


void ltdc_init(void){
	// Step 0: Init GPIOs for LTDC signals (RGB, HSYNC, VSYNC, DE, CLK)
	ltdc_gpio_init();

	// Step 1: Enable LTDC clock on APB2
	rcc_enable_apb2_clock(RCC_APB2EN_LTDC);
	rcc_restart_apb2_peripheral(RCC_APB2RSTR_LTDC);

	// Step 2: Configure PLLSAI for LCD pixel clock
	//PLLSAI for LTDC: N=50, R=2, DIVR=2  => LCD_CLK ≈ 6.25 MHz */
	rcc_pllsaicfgr_write_field(RCC_PLLSAICFGR_PLLSAIN, 50U);
	rcc_pllsaicfgr_write_field(RCC_PLLSAICFGR_PLLSAIR, 0x2U);
	rcc_dckcfgr_write_field(RCC_DCKCFGR_PLLSAIDIVR, 0x2U);
	rcc_cr_write_field(RCC_CR_PLLSAION, 1U);
	while (rcc_cr_read_field(RCC_CR_PLLSAIRDY)==0U) {}

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
	ltdc_gcr_set_field(LTDC_GCR_FIELD_DEPOL, 0); // DE active low
	ltdc_gcr_set_field(LTDC_GCR_FIELD_VSPOL, 0); // VSYNC active low
	ltdc_gcr_set_field(LTDC_GCR_FIELD_HSPOL, 0); // HSYNC active low
    ltdc_gcr_set_field(LTDC_GCR_FIELD_PCPOL, 0); // Pixel clock polarity: normal (Latch on rising DOTCLK)

    // Step 5: Set background color
    ltdc_bccr_set_rgb565_color(255, 0, 0);  // Red background

    // Step 6: (Optional) Setup interrupts

    // Step 7: Configure Layer 1 (foreground)
    // 7.1 Set Layer 1 window position within the active display area (defined by AWCR)
    // ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSTPOS, LTDC_LAYER_WHSTPOS);  // Horizontal start = AHBP + 1 (first visible pixel)
    // ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSPPOS, LTDC_LAYER_WHSPPOS);  // Horizontal stop  = AAW (last visible pixel)
    ltdc_lxwhpcr_set_window(1, LTDC_LAYER_WHSTPOS, LTDC_LAYER_WHSPPOS);
    // ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSTPOS, LTDC_LAYER_WVSTPOS);  // Vertical start = AVBP + 1 (first visible line)
    // ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSPPOS, LTDC_LAYER_WVSPPOS);  // Vertical stop  = AAH (last visible line)
    ltdc_lxwvpcr_set_window(1, LTDC_LAYER_WVSTPOS, LTDC_LAYER_WVSPPOS);

    // 7.2 Set pixel format for Layer 1 (RGB565 = 16-bit color)
    ltdc_lxpfcr_set_field(1, LTDC_PIXEL_FORMAT_RGB565);

    // 7.3 Set frame buffer start address for Layer 1
    ltdc_lxcfbar_set_field(1, FRAMEBUFFER_ADDR);

    // 7.4 Set frame buffer line length and pitch (in bytes)
	// ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBLL, LTDC_ACTIVE_WIDTH); // Line length = width * bytes per pixel + 3
    // ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBP, LTDC_ACTIVE_WIDTH);  // Pitch = width * bytes per pixel
    ltdc_lxcfblr_set_window(1, LTDC_ACTIVE_WIDTH, LTDC_ACTIVE_WIDTH);

    // 7.5 Set number of active display lines (height)
    ltdc_lxcfblnr_set_field(1, LTDC_ACTIVE_HEIGHT);


    // ltdc_lxbfcr_set_field(1, 0x7, 0x7);
    ltdc_lxcacr_set_field(1, 255);
    ltdc_lxbfcr_set_field(1, 0x4, 0x5);


    // Step 8: Enable Layer 1, disable Layer 2
    ltdc_lxcr_set_field(1, LTDC_LXCR_FIELD_LEN, 1);  // Enable Layer 1
    ltdc_srcr_set_field(LTDC_SRCR_FIELD_IMR, 1);
    ltdc_lxcr_set_field(2, LTDC_LXCR_FIELD_LEN, 0);  // Disable Layer 2


    // Step 9: Reload shadow registers immediately
    ltdc_srcr_set_field(LTDC_SRCR_FIELD_IMR, 1);


    // Step 10: Enable LTDC controller (LTDC_GCR bit 0 = 1)
    ltdc_gcr_set_field(LTDC_GCR_FIELD_LTDCEN, 1);
    ltdc_srcr_set_field(LTDC_SRCR_FIELD_IMR, 1);
}

void bsp_lcd_fill_rect(uint32_t rgb888,
                       uint32_t x_start, uint32_t x_width,
                       uint32_t y_start, uint32_t y_height)
{
    /* 畫面大小（可直接宏替換成 BSP_FB_WIDTH/HEIGHT） */
    const uint32_t FB_W = LTDC_ACTIVE_WIDTH;
    const uint32_t FB_H = LTDC_ACTIVE_HEIGHT;

    /* 邊界檢查與裁切（避免越界） */
    if (x_start >= FB_W || y_start >= FB_H) return;
    if (x_start + x_width  > FB_W) x_width  = FB_W - x_start;
    if (y_start + y_height > FB_H) y_height = FB_H - y_start;

    /* 顏色轉成 RGB565 一次就好 */
    const uint16_t c565 = Convert_RGB888_to_RGB565(rgb888);

    /* 逐行填色：每行的起始位址 = base + (y*FB_W + x) */
    while (y_height--) {
        uint32_t pixel_index = y_start * FB_W + x_start;
        uint16_t *line_ptr = (uint16_t *)FRAMEBUFFER_ADDR + pixel_index;

        write_to_fb_rgb565(line_ptr, x_width, c565);
        y_start++;
    }
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
}  // be error from io_writeMask

void ltdc_lxwhpcr_set_window(uint8_t layerx, uint16_t whstpos, uint16_t whsppos) {
    uint32_t addr = LTDC_BASE + LTDC_LXWHPCR_OFFSET(layerx);
    uint32_t val = ((uint32_t)(whsppos & 0x0FFF) << 16) | (whstpos & 0x0FFF);
    io_write(addr, val);
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

void ltdc_lxwvpcr_set_window(uint8_t layerx, uint16_t wvstpos, uint16_t wvsppos) {
	uint32_t addr = LTDC_BASE + LTDC_LXWVPCR_OFFSET(layerx);
    uint32_t val  = ((uint32_t)(wvsppos & 0x0FFF) << 16) | (wvstpos & 0x0FFF);

    io_write(addr, val);
}

void ltdc_lxpfcr_set_field(uint8_t layerx, uint32_t pixel_format) {
    uint32_t addr = LTDC_BASE + LTDC_LXPFCR_OFFSET(layerx);
    uint32_t mask = 0x07U;
    uint32_t data = pixel_format & mask;
    io_writeMask(addr, data, mask);
}

void ltdc_lxcfbar_set_field(uint8_t layerx, uint32_t ram_addr) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBAR_OFFSET(layerx);
    io_write(addr, ram_addr);
    //io_writeMask(addr, ram_addr, 0xFFFFFFFF);
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

void ltdc_lxcfblr_set_window(uint8_t layerx, uint16_t cfbllpos, uint16_t cfbppos) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLR_OFFSET(layerx);
    uint32_t val  = ((uint32_t)((cfbppos * 2) & 0x1FFF) << 16) | ((cfbllpos * 2 + 3) & 0x1FFF);

    io_write(addr, val);
}

void ltdc_lxcfblnr_set_field(uint8_t layerx, uint32_t height) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLNR_OFFSET(layerx);
    uint32_t mask = 0x7FF;
    io_writeMask(addr, height, mask);
}

// Constant Alpha (8-bit, 0~255)
void ltdc_lxcacr_set_field(uint8_t layerx, uint8_t const_alpha)
{
    uint32_t addr = LTDC_BASE + LTDC_LXCACR_OFFSET(layerx);
    uint32_t mask = 0xFFu;                      // bits [7:0]
    uint32_t data = ((uint32_t)const_alpha) & mask;
    io_writeMask(addr, data, mask);
}

// Blending Factors
// BF1 -> bits [10:8], BF2 -> bits [2:0]
// 常見組合：BF1=0x4（CA * px color）/ BF2=0x5（(1-CA) * bg color）
void ltdc_lxbfcr_set_field(uint8_t layerx, uint32_t bf1, uint32_t bf2)
{
    uint32_t addr = LTDC_BASE + LTDC_LXBFCR_OFFSET(layerx);
    uint32_t mask = (0x7u << 8) | 0x7u;         // BF1[10:8] + BF2[2:0]
    uint32_t data = ((bf1 & 0x7u) << 8) | (bf2 & 0x7u);
    io_writeMask(addr, data, mask);
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


