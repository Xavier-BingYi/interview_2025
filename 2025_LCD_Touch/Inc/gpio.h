/*
 * gpio.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <mem_map.h>

/* GPIO pins */
#define GPIO_PIN_0   0
#define GPIO_PIN_1   1
#define GPIO_PIN_2   2
#define GPIO_PIN_3   3
#define GPIO_PIN_4   4
#define GPIO_PIN_5   5
#define GPIO_PIN_6   6
#define GPIO_PIN_7   7
#define GPIO_PIN_8   8
#define GPIO_PIN_9   9
#define GPIO_PIN_10 10
#define GPIO_PIN_11 11
#define GPIO_PIN_12 12
#define GPIO_PIN_13 13
#define GPIO_PIN_14 14
#define GPIO_PIN_15 15

/* GPIO MODER register bit values (2-bit per pin) */
#define GPIO_MODE_INPUT      0U  // 00: Input mode
#define GPIO_MODE_OUTPUT     1U  // 01: General purpose output mode
#define GPIO_MODE_ALTERNATE  2U  // 10: Alternate function mode
#define GPIO_MODE_ANALOG     3U  // 11: Analog mode (power save / ADC)

/* GPIO alternate function (AF0 ~ AF15) */
typedef enum {
    ALTERNATE_AF0  = 0,  // SYS
    ALTERNATE_AF1  = 1,  // TIM1 / TIM2
    ALTERNATE_AF2  = 2,  // TIM3 / TIM4 / TIM5
    ALTERNATE_AF3  = 3,  // TIM8 / TIM9 / TIM10 / TIM11
    ALTERNATE_AF4  = 4,  // I2C1 / I2C2 / I2C3
    ALTERNATE_AF5  = 5,  // SPI1 / SPI2 / SPI3 / SPI4 / SPI5 / SPI6
    ALTERNATE_AF6  = 6,  // SPI2 / SPI3 / SAI1
    ALTERNATE_AF7  = 7,  // SPI3 / USART1 / USART2 / USART3
    ALTERNATE_AF8  = 8,  // USART6 / UART4 / UART5 / UART7 / UART8
    ALTERNATE_AF9  = 9,  // CAN1 / CAN2 / TIM12 / TIM13 / TIM14 / LCD
    ALTERNATE_AF10 = 10, // OTG2_HS / OTG1_FS
    ALTERNATE_AF11 = 11, // ETH
    ALTERNATE_AF12 = 12, // FMC / SDIO / OTG2_FS
    ALTERNATE_AF13 = 13, // DCMI
    ALTERNATE_AF14 = 14, // LCD
    ALTERNATE_AF15 = 15  // SYS
} GPIO_AlternateFunction;

typedef enum {
    GPIO_SPEED_LOW = 0,      // 00
    GPIO_SPEED_MEDIUM,       // 01
    GPIO_SPEED_HIGH,         // 10
    GPIO_SPEED_VERY_HIGH     // 11
} gpio_ospeedr_field_t;



void gpio_init(void);
void gpio_set_mode(uint32_t port_base, uint8_t pin, uint8_t mode);
void gpio_set_alternate_function(uint32_t port_base, uint8_t pin, GPIO_AlternateFunction func);
void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val);
void gpio_set_speed(uint32_t gpio_base, uint8_t pin, gpio_ospeedr_field_t speed);

#endif /* GPIO_H_ */
