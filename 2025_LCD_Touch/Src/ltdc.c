/*
 * ltdc.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <mem_io.h>

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

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE); // R1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_0, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // R2
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // R3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_12, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE); // R4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_1, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // R5
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    // GREEN pins (G0~G5)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // G0
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G2
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // G3
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE); // G4
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_7, ALTERNATE_AF14);

    gpio_set_mode(GPIOD_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE); // G5
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_3, ALTERNATE_AF14);

    // BLUE pins (B0~B5)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // B0
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // B1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // B2
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_12, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE); // B3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_3, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE); // B4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_8, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE); // B5
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_9, ALTERNATE_AF14);

    // LCD control signals
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // DE
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // DOTCLK
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_7, ALTERNATE_AF14);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // HSYNC
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);  // VSYNC
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_4, ALTERNATE_AF14);
}

