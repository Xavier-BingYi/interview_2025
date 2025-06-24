/*
 * usart.h
 *
 *  Created on: Jun 11, 2025
 *      Author: Xavier
 */

#ifndef USART_H_
#define USART_H_

#include <stdbool.h>

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
#define USART_SR_OFFSET  0x00  // Status register
#define USART_DR_OFFSET  0x04  // Data register
#define USART_BRR_OFFSET 0x08  // Baud rate register
#define USART_CR1_OFFSET 0x0C  // Control register 1
#define USART_CR2_OFFSET 0x10  // Control register 2

/* USART SR bit position */
#define USART_SR_TXE 7
#define USART_SR_TC  6

/* USART CR1 bit position */
#define USART_CR1_RE 2
#define USART_CR1_TE 3
#define USART_CR1_M 12
#define USART_CR1_UE 13
#define USART_CR1_OVER8 15

/* USART CR2 bit position */
#define USART_CR2_STOP 12
typedef enum {
    USART_STOP_1     = 0b00,
    USART_STOP_0_5   = 0b01,
    USART_STOP_2     = 0b10,
    USART_STOP_1_5   = 0b11
} usart_stop_bits_t;


void usart_init(void);
void usart_rcc_enable(USART_RCC_Module usart_num);
void usart_brr(uint32_t usart_base,  uint32_t usart_div_x100);
void usart_set_baudrate(uint32_t usart_base, uint32_t fck, uint32_t baudrate);
bool usart_SR_read_bit(uint32_t usart_base, uint8_t bit_pos);
void usart_cr1_write_bit(uint32_t usart_base, uint8_t bit_pos, uint8_t val);
void usart_cr2_write_bit(uint32_t usart_base, uint8_t bit_pos, uint8_t val);
void usart_cr2_write_bits(uint32_t usart_base, uint8_t bit_pos, uint8_t width, uint8_t val);
void usart_write(uint32_t usart_base, uint8_t data);
void usart_wait_complete(uint32_t usart_base);
void usart_print(uint32_t usart_base, const char *str);
void usart_printf(const char *fmt, ...);


#endif /* USART_H_ */
