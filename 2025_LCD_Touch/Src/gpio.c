/*
 * gpio.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <rcc.h>
#include <gpio.h>


void gpio_init(uint8_t port){
	rcc_enable_ahb1_clock(port);
}

void gpio_set_mode(uint32_t port_base, uint8_t pin, uint8_t mode){
	uint32_t reg_addr = port_base + GPIO_MODER_OFFSET;
	uint32_t shift = (pin & 0x0F)* 2;
	uint32_t data = ((uint32_t)mode & 3U) << shift;
	uint32_t mask = 3U << shift;
	io_writeMask(reg_addr, data, mask);
}

void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val){
	uint32_t reg_addr = port_base + GPIO_ODR_OFFSET;
	uint32_t shift = (pin & 0x0F);
	uint32_t data = ((uint32_t)val & 1U) << shift;
	uint32_t mask = 1U << shift;
	io_writeMask(reg_addr, data, mask);
}
