/*
 * RCC.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef RCC_H_
#define RCC_H_

#define RCC_BASE       0x40023800UL

/* RCC register address offset */
#define RCC_AHB1ENR    0x30
#define RCC_APB1ENR    0x40
#define RCC_APB2ENR    0x44

/* RCC AHB1 bit positions */
#define RCC_AHB1EN_GPIOA 0
#define RCC_AHB1EN_GPIOB 1
#define RCC_AHB1EN_GPIOC 2
#define RCC_AHB1EN_GPIOD 3
#define RCC_AHB1EN_GPIOE 4
#define RCC_AHB1EN_GPIOF 5
#define RCC_AHB1EN_GPIOG 6
#define RCC_AHB1EN_GPIOH 7
#define RCC_AHB1EN_GPIOI 8
#define RCC_AHB1EN_GPIOJ 9
#define RCC_AHB1EN_GPIOK 10

/* RCC USARTEN Group */
typedef enum {
    RCC_USART1EN,
    RCC_USART2EN,
    RCC_USART3EN,
    RCC_UART4EN,
    RCC_UART5EN,
    RCC_USART6EN,
    RCC_UART7EN,
    RCC_UART8EN,
} USART_RCC_Module;


void rcc_enable_ahb1_clock(uint8_t bit_pos);
void rcc_enable_apb1_clock(uint8_t bit_pos);
void rcc_enable_apb2_clock(uint8_t bit_pos);


#endif /* RCC_H_ */
