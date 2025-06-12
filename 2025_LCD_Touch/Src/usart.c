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
}

void usart_rcc_enable(USART_Module usart_num){
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
		default:
			return;
	}
}
