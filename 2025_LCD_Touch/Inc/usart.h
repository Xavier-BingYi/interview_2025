/*
 * usart.h
 *
 *  Created on: Jun 11, 2025
 *      Author: Xavier
 */

#ifndef USART_H_
#define USART_H_

#include <stdbool.h>
#include <mem_io.h>
#include <mem_map.h>

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
