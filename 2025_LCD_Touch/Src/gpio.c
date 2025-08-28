/*
 * gpio.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <stdbool.h>
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

bool gpio_read_idr(uint32_t gpio_base_addr, uint8_t pin) {
    if (pin >= 16) return false;
    uint32_t data = io_read(gpio_base_addr + GPIO_IDR_OFFSET);
    return ((data >> pin) & 0x01) != 0;
}

void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val){
	uint32_t reg_addr = port_base + GPIO_ODR_OFFSET;
	uint32_t shift = (pin & 0x0F);
	uint32_t data = ((uint32_t)val & 1U) << shift;
	uint32_t mask = 1U << shift;
	io_writeMask(reg_addr, data, mask);
}

void gpio_set_speed(uint32_t gpio_base, uint8_t pin, gpio_ospeedr_field_t speed)
{
    if (pin > 15) return;
    uint32_t addr  = gpio_base + GPIO_OSPEEDR_OFFSET;
    uint32_t shift = (uint32_t)pin * 2U;
    uint32_t mask  = (0x3U << shift);
    uint32_t data  = ((uint32_t)speed << shift);
    io_writeMask(addr, data, mask);
}

void gpio_set_output_type(uint32_t gpio_base, uint8_t pin, gpio_otype_t type)
{
    if (pin > 15) return;
    uint32_t addr  = gpio_base + GPIO_OTYPER_OFFSET;
    uint32_t shift = (uint32_t)pin;        // 1-bit field per pin
    uint32_t mask  = (0x1U << shift);
    uint32_t data  = ((uint32_t)type << shift);
    io_writeMask(addr, data, mask);
}

void gpio_set_pupdr(uint32_t gpio_base, uint8_t pin, gpio_pupdr_t type) {
    if (pin > 15) return;
    uint32_t addr  = gpio_base + GPIO_PUPDR_OFFSET;
    uint32_t shift = (uint32_t)pin * 2;
    uint32_t mask  = (0x3U << shift);
    uint32_t data  = ((uint32_t)type << shift);
    io_writeMask(addr, data, mask);
}
