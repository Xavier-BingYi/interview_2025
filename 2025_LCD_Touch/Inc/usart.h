/*
 * usart.h
 *
 *  Created on: Jun 11, 2025
 *      Author: Xavier
 */

#ifndef USART_H_
#define USART_H_

/* USART Group */
//---------------------- APB2 ----------------------
#define USART1_BASE (APB2PERIPH_BASE + 0x1000)  // APB2
#define USART6_BASE (APB2PERIPH_BASE + 0x1400)  // APB2

//---------------------- APB1 ----------------------
#define USART2_BASE (APB1PERIPH_BASE + 0x4400)  // APB1
#define USART3_BASE (APB1PERIPH_BASE + 0x4800)  // APB1
#define UART4_BASE  (APB1PERIPH_BASE + 0x4C00)  // APB1
#define UART5_BASE  (APB1PERIPH_BASE + 0x5000)  // APB1
#define UART7_BASE  (APB1PERIPH_BASE + 0x7800)  // APB1
#define UART8_BASE  (APB1PERIPH_BASE + 0x7C00)  // APB1

/* USART register offsets (from USARTx_BASE)*/
#define USART_BRR_OFFSET 0x08  // Baud rate register
#define USART_CR1_OFFSET 0x0C  // Control register 1

/* USART CR1 bit position */
#define USART_CR1_RE 2
#define USART_CR1_TE 3
#define USART_CR1_M 12
#define USART_CR1_UE 13
#define USART_CR1_OVER8 15


void usart_rcc_enable(uint8_t usart_num);
void usart_init(void);
void usart_brr(uint32_t usart_base,  uint32_t usart_div_x100);
void usart_set_baudrate(uint32_t usart_base, uint32_t fck, uint32_t baudrate);
void usart_cr1_write_bit(uint32_t usart_base, uint8_t bit_pos, uint8_t val);


#endif /* USART_H_ */
