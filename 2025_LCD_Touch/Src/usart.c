/*
 * usart.c
 *
 *  Created on: Jun 11, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <mem_io.h>
#include <usart.h>

void usart_init(void){
	usart_rcc_enable(RCC_USART1EN);

	gpio_set_mode(GPIOA_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
	gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_9, ALTERNATE_AF7);  // USART1_TX

	gpio_set_mode(GPIOA_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE);
	gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_10, ALTERNATE_AF7); // USART1_RX

	usart_cr1_write_bit(USART1_BASE, USART_CR1_OVER8, 0);
	usart_set_baudrate(USART1_BASE, FCK_APB2, 9600);
}

void usart_rcc_enable(USART_RCC_Module usart_num){
	switch (usart_num){
		case RCC_USART1EN:
			rcc_enable_apb2_clock(4);
			break;
		case RCC_USART6EN:
			rcc_enable_apb2_clock(5);
			break;
		case RCC_USART2EN:
			rcc_enable_apb1_clock(17);
			break;
		case RCC_USART3EN:
			rcc_enable_apb1_clock(18);
			break;
		case RCC_UART4EN:
			rcc_enable_apb1_clock(19);
			break;
		case RCC_UART5EN:
			rcc_enable_apb1_clock(20);
			break;
		case RCC_UART7EN:
			rcc_enable_apb1_clock(30);
			break;
		case RCC_UART8EN:
			rcc_enable_apb1_clock(30);
			break;
		default:
			return;
	}
}

void usart_brr(uint32_t usart_base,  uint32_t usart_div_x100){
	uint32_t reg_addr = usart_base + USART_BRR_OFFSET;

	uint32_t cr1_over8 = (io_read(usart_base + USART_CR1_OFFSET) >> USART_CR1_OVER8) & 0x01;

	uint32_t mantissa = usart_div_x100 / 100;
	uint32_t fraction = usart_div_x100 - mantissa * 100;

	if (cr1_over8 == 0){
		fraction = (fraction * 16 + 50) / 100;   // (fraction / 100) * 16 = 0 → use (fraction * 16 + 50) / 100
	}else{
		fraction = ((fraction * 8 + 50) / 100) & 0x07;  // (fraction / 100) * 8 = 0 → use (fraction * 8 + 50) / 100
	}

	uint32_t val = (mantissa << 4) | fraction;
	io_write(reg_addr, val);
}

void usart_set_baudrate(uint32_t usart_base, uint32_t fck, uint32_t baudrate){
	uint32_t cr1_over8 = (io_read(usart_base + USART_CR1_OFFSET) >> USART_CR1_OVER8) & 0x01;

	uint32_t usart_div_x100 = (fck*100) / (baudrate * 8 * (2 - cr1_over8));

	usart_brr(usart_base, usart_div_x100);
}

void usart_cr1_write_bit(uint32_t usart_base, uint8_t bit_pos, uint8_t val){
	uint32_t reg_addr = usart_base + USART_CR1_OFFSET;
	uint32_t mask = 1U << bit_pos;
	uint32_t data = (uint32_t)val << bit_pos;
	io_writeMask(reg_addr, data, mask);
}




