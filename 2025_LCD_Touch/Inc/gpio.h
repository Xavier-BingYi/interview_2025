/*
 * gpio.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <mem_map.h>

/* GPIO port register base addresses (AHB1) */
#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE (AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASE (AHB1PERIPH_BASE + 0x1400)
#define GPIOG_BASE (AHB1PERIPH_BASE + 0x1800)
#define GPIOH_BASE (AHB1PERIPH_BASE + 0x1C00)
#define GPIOI_BASE (AHB1PERIPH_BASE + 0x2000)
#define GPIOJ_BASE (AHB1PERIPH_BASE + 0x2400)
#define GPIOK_BASE (AHB1PERIPH_BASE + 0x2800)

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

/* GPIO register offsets (from GPIOx_BASE)*/
#define GPIO_MODER_OFFSET    0x00  // Mode register
//#define GPIO_OTYPER_OFFSET   0x04  // Output type register
//#define GPIO_OSPEEDR_OFFSET  0x08  // Output speed register
//#define GPIO_PUPDR_OFFSET    0x0C  // Pull-up/pull-down register
//#define GPIO_IDR_OFFSET      0x10  // Input data register
#define GPIO_ODR_OFFSET      0x14  // Output data register
//#define GPIO_BSRR_OFFSET     0x18  // Bit set/reset register
//#define GPIO_AFRL_OFFSET     0x20  // Alternate function low register
//#define GPIO_AFRH_OFFSET     0x24  // Alternate function high register

/* GPIO MODER register bit values (2-bit per pin) */
#define GPIO_MODE_INPUT      0U  // 00: Input mode
#define GPIO_MODE_OUTPUT     1U  // 01: General purpose output mode
#define GPIO_MODE_ALTERNATE  2U  // 10: Alternate function mode
#define GPIO_MODE_ANALOG     3U  // 11: Analog mode (power save / ADC)

void gpio_init(uint8_t port);
void gpio_set_mode(uint32_t port_base, uint8_t pin, uint8_t mode);
void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val);


#endif /* GPIO_H_ */
