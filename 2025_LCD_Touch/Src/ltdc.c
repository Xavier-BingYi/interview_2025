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
    // 以 PJ13、PJ14、PK6、PI10 等為例（依 ST 官網 Pinout 為主）
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOJ);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOK);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOI);

    gpio_set_mode(GPIOJ_BASE, GPIO_PIN_13, GPIO_MODE_AF);
    gpio_set_alternate_function(GPIOJ_BASE, GPIO_PIN_13, GPIO_AF_LTDC);

    // ...其餘所有 LTDC 腳位一樣處理（PJ, PK, PI 共十幾條 RGB 資料線）
}
