/*
 * sdram.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <mem_io.h>

void sdram_gpio_init(void)
{
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOE);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOH);

    gpio_set_mode(GPIOD_BASE, GPIO_PIN_0, GPIO_MODE_AF);
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_0, GPIO_AF_FMC);

    // ...所有地址線、資料線、控制線也都同樣處理
}
