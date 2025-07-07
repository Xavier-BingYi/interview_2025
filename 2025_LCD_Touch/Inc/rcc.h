/*
 * RCC.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef RCC_H_
#define RCC_H_

/* USART bus frequency（MCU default value）*/
#define FCK_APB2 16000000U  // USART1/6

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

/* RCC APB2 bit positions */
#define RCC_APB2EN_SYSCFG 14  // SYSCFG EN bit = bit 14

/* AHB3 peripheral clock enable bit for FMC controller */
#define RCC_AHB3ENR_FMCEN (1U << 0)


void rcc_enable_ahb1_clock(uint8_t bit_pos);
void rcc_enable_ahb3_clock(void);
void rcc_enable_apb1_clock(uint8_t bit_pos);
void rcc_enable_apb2_clock(uint8_t bit_pos);


#endif /* RCC_H_ */
