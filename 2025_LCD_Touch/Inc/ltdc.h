/*
 * ltdc.h
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */

#ifndef LTDC_H_
#define LTDC_H_

//LTDC Timing Configuration Parameters
#define LTDC_ACTIVE_WIDTH 240   // Active Pixels per Line
#define LTDC_ACTIVE_HEIGHT 320  // Active Lines per Frame

#define LTDC_HSYNC        10    // Horizontal Sync Pulse Width (HSW)
#define LTDC_HBP          20    // Horizontal Back Porch (HBP)
#define LTDC_HFP          10    // Horizontal Front Porch (HFP)

#define LTDC_VSYNC        2     // Vertical Sync Height (VSH)
#define LTDC_VBP          2     // Vertical Back Porch (VBP)
#define LTDC_VFP          4     // Vertical Front Porch (VFP)

// Derived Register Values (Based on RM0090 Formulas)
// Synchronization Size Configuration Register (SSCR)
#define LTDC_SSCR_HSW     (LTDC_HSYNC  - 1)
#define LTDC_SSCR_VSH     (LTDC_VSYNC  - 1)

// Back Porch Configuration Register (BPCR)
#define LTDC_BPCR_AHBP    (LTDC_HSYNC + LTDC_HBP - 1)
#define LTDC_BPCR_AVBP    (LTDC_VSYNC + LTDC_VBP - 1)

// Active Width Configuration Register (AWCR)
#define LTDC_AWCR_AAW     (LTDC_HSYNC + LTDC_HBP + LTDC_ACTIVE_WIDTH - 1)
#define LTDC_AWCR_AAH     (LTDC_VSYNC + LTDC_VBP + LTDC_ACTIVE_HEIGHT - 1)

// Total Width Configuration Register (TWCR)
#define LTDC_TWCR_TOTALW  (LTDC_HSYNC + LTDC_HBP + LTDC_ACTIVE_WIDTH + LTDC_HFP - 1)
#define LTDC_TWCR_TOTALH  (LTDC_VSYNC + LTDC_VBP + LTDC_ACTIVE_HEIGHT + LTDC_VFP - 1)

// Layer Window Horizontal Position Configuration Register (LxWHPCR)
#define LTDC_LAYER_WHSTPOS  (LTDC_BPCR_AHBP + 1)
#define LTDC_LAYER_WHSPPOS  (LTDC_AWCR_AAW)

// Layer Window Vertical Position Configuration Register (LxWVPCR)
#define LTDC_LAYER_WVSTPOS  (LTDC_BPCR_AVBP + 1)
#define LTDC_LAYER_WVSPPOS  (LTDC_AWCR_AAH)

#define LTDC_PIXEL_FORMAT_RGB888     0x1
#define LTDC_PIXEL_FORMAT_RGB565     0x2

typedef enum {
    LTDC_SSCR_FIELD_HSW,   // Horizontal sync width (bits [27:16])
    LTDC_SSCR_FIELD_VSH    // Vertical sync height (bits [10:0])
} ltdc_sscr_field_t;

typedef enum {
    LTDC_BPCR_FIELD_AVBP,  // Vertical back porch (bits [10:0])
    LTDC_BPCR_FIELD_AHBP   // Horizontal back porch (bits [27:16])
} ltdc_bpcr_field_t;

typedef enum {
    LTDC_AWCR_FIELD_AAH,   // Active height (bits [10:0])
    LTDC_AWCR_FIELD_AAW    // Active width (bits [27:16])
} ltdc_awcr_field_t;

typedef enum {
    LTDC_TWCR_FIELD_TOTALH, // Total height (bits [10:0])
    LTDC_TWCR_FIELD_TOTALW  // Total width (bits [27:16])
} ltdc_twcr_field_t;

typedef enum {
    LTDC_GCR_FIELD_LTDCEN = 0,   // Bit 0 : LCD-TFT controller enable
    LTDC_GCR_FIELD_DBW    = 4,   // Bit 6:4 : Dither Blue Width (read-only)
    LTDC_GCR_FIELD_DGW    = 8,   // Bit 10:8 : Dither Green Width (read-only)
    LTDC_GCR_FIELD_DRW    = 12,  // Bit 14:12 : Dither Red Width (read-only)
    LTDC_GCR_FIELD_DEN    = 16,  // Bit 16 : Dither Enable
    LTDC_GCR_FIELD_PCPOL  = 28,  // Bit 28 : Pixel Clock Polarity
	LTDC_GCR_FIELD_DEPOL  = 29,  // Bit 29 : Data Enable Polarity
    LTDC_GCR_FIELD_VSPOL  = 30,  // Bit 30 : Vertical Sync Polarity
    LTDC_GCR_FIELD_HSPOL  = 31   // Bit 31 : Horizontal Sync Polarity
} ltdc_gcr_field_t;

typedef enum {
    LTDC_LXCR_FIELD_LEN    = 0,
    LTDC_LXCR_FIELD_COLKEN = 1,
    LTDC_LXCR_FIELD_CLUTEN = 4
} ltdc_lxcr_field_t;

typedef enum {
    LTDC_LXWHPCR_FIELD_WHSTPOS,
    LTDC_LXWHPCR_FIELD_WHSPPOS
} ltdc_lxwhpcr_field_t;

typedef enum {
    LTDC_LXWVPCR_FIELD_WVSTPOS,  // Window Vertical Start Position
    LTDC_LXWVPCR_FIELD_WVSPPOS   // Window Vertical Stop Position
} ltdc_lxwvpcr_field_t;

typedef enum {
    LTDC_LXCFBLR_FIELD_CFBLL,  // Color Frame Buffer Line Length
    LTDC_LXCFBLR_FIELD_CFBP    // Color Frame Buffer Pitch
} ltdc_lxcfblr_field_t;

typedef enum {
    LTDC_SRCR_FIELD_IMR = 0,  // Immediate Reload
    LTDC_SRCR_FIELD_VBR = 1   // Vertical Blanking Reload
} ltdc_srcr_field_t;





void ltdc_gpio_init(void);
void ltdc_init(void);
void ltdc_sscr_set_field(ltdc_sscr_field_t field, uint32_t value);
void ltdc_bpcr_set_field(ltdc_bpcr_field_t field, uint32_t value);
void ltdc_awcr_set_field(ltdc_awcr_field_t field, uint32_t value);
void ltdc_twcr_set_field(ltdc_twcr_field_t field, uint32_t value);
void ltdc_gcr_set_field(ltdc_gcr_field_t field, uint32_t value);
uint8_t to_rgb565_r(uint8_t r8);
uint8_t to_rgb565_g(uint8_t g8);
uint8_t to_rgb565_b(uint8_t b8);
void ltdc_bccr_set_rgb565_color(uint8_t red, uint8_t green, uint8_t blue);
void ltdc_lxwhpcr_set_field(uint8_t layerx, ltdc_lxwhpcr_field_t field, uint32_t value);
void ltdc_lxwhpcr_set_window(uint8_t layerx, uint16_t whstpos, uint16_t whsppos);
void ltdc_lxwvpcr_set_field(uint8_t layerx, ltdc_lxwvpcr_field_t field, uint32_t value);
void ltdc_lxwvpcr_set_window(uint8_t layerx, uint16_t wvstpos, uint16_t wvsppos);
void ltdc_lxpfcr_set_field(uint8_t layerx, uint32_t pixel_format);
void ltdc_lxcfbar_set_field(uint8_t layerx, uint32_t ram_addr);
void ltdc_lxcfblr_set_field(uint8_t layerx, ltdc_lxcfblr_field_t field, uint32_t lcd_width);
void ltdc_lxcfblr_set_window(uint8_t layerx, uint16_t cfbllpos, uint16_t cfbppos);
void ltdc_lxcfblnr_set_field(uint8_t layerx, uint32_t height);
void ltdc_lxcacr_set_field(uint8_t layerx, uint8_t const_alpha);
void ltdc_lxbfcr_set_field(uint8_t layerx, uint32_t bf1, uint32_t bf2);
void ltdc_lxcr_set_field(uint8_t layerx, ltdc_lxcr_field_t field, uint32_t value);
void ltdc_srcr_set_field(ltdc_srcr_field_t field, uint32_t value);


void fill_framebuffer_rgb888(uint32_t rgb888);
void bsp_lcd_fill_rect(uint32_t rgb888, uint32_t x_start, uint32_t x_width, uint32_t y_start, uint32_t y_height);



#endif /* LTDC_H_ */
