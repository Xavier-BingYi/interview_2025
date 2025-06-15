/*
 * gpio.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <mem_io.h>



void gpio_init(void){
	rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

	gpio_set_mode(GPIOG_BASE, GPIO_PIN_13, GPIO_MODE_OUTPUT);
	gpio_set_mode(GPIOG_BASE, GPIO_PIN_14, GPIO_MODE_OUTPUT);
}

void gpio_set_mode(uint32_t port_base, uint8_t pin, uint8_t mode){
	uint32_t reg_addr = port_base + GPIO_MODER_OFFSET;
	uint32_t shift = (pin & 0x0F)* 2;
	uint32_t data = ((uint32_t)mode & 3U) << shift;
	uint32_t mask = 3U << shift;
	io_writeMask(reg_addr, data, mask);
}

void gpio_set_alternate_function(uint32_t port_base, uint8_t pin, GPIO_AlternateFunction af){
    uint32_t reg_addr;
    uint32_t shift;
    uint32_t data;
    uint32_t mask;

    if (pin <= 7){
		reg_addr = port_base + GPIO_AFRL_OFFSET;
		shift = pin* 4;
	}else if (pin <= 15){
		reg_addr = port_base + GPIO_AFRH_OFFSET;
		shift = (pin - 8)* 4;
	}else{
		return;
	}

    data = ((uint32_t)af & 0x0F) << shift;
    mask = 0x0F << shift;

	io_writeMask(reg_addr, data, mask);
}

void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val){
	uint32_t reg_addr = port_base + GPIO_ODR_OFFSET;
	uint32_t shift = (pin & 0x0F);
	uint32_t data = ((uint32_t)val & 1U) << shift;
	uint32_t mask = 1U << shift;
	io_writeMask(reg_addr, data, mask);
}
