/*
 * mem_map.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Xavier
 */

#ifndef MEM_MAP_H_
#define MEM_MAP_H_

#define FRAMEBUFFER_ADDR  (0x20000000 + 0x5400)


#define AHB1PERIPH_BASE 0x40020000
#define APB2PERIPH_BASE 0x40010000
#define APB1PERIPH_BASE 0x40000000




/* RCC Group */
//------------------------------------------------
#define RCC_BASE       0x40023800UL

/* RCC register address offset */
#define RCC_CR         0x00
#define RCC_PLLCFGR    0x04
#define RCC_CFGR       0x08
#define RCC_APB2RSTR   0x24
#define RCC_AHB1ENR    0x30
#define RCC_AHB3ENR    0x38
#define RCC_APB1ENR    0x40
#define RCC_APB2ENR    0x44
#define RCC_PLLSAICFGR 0x88
#define RCC_DCKCFGR    0x8C





/* GPIO Group */
//------------------------------------------------
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

/* GPIO register offsets (from GPIOx_BASE)*/
#define GPIO_MODER_OFFSET    0x00  // Mode register
#define GPIO_OTYPER_OFFSET   0x04  // Output type register
#define GPIO_OSPEEDR_OFFSET  0x08  // Output speed register
//#define GPIO_PUPDR_OFFSET    0x0C  // Pull-up/pull-down register
#define GPIO_IDR_OFFSET      0x10  // Input data register
#define GPIO_ODR_OFFSET      0x14  // Output data register
//#define GPIO_BSRR_OFFSET     0x18  // Bit set/reset register
#define GPIO_AFRL_OFFSET     0x20  // Alternate function low register
#define GPIO_AFRH_OFFSET     0x24  // Alternate function high register




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





/* SYSCFG Group */
//------------------------------------------------
#define SYSCFG_BASE 0x40013800

/* SYSCFG register offsets (relative to SYSCFG_BASE) */
#define SYSCFG_EXTICR1_OFFSET    0x08  // External interrupt configuration register 1 (EXTI0–3)
#define SYSCFG_EXTICR2_OFFSET    0x0C  // External interrupt configuration register 2 (EXTI4–7)
#define SYSCFG_EXTICR3_OFFSET    0x10  // External interrupt configuration register 3 (EXTI8–11)
#define SYSCFG_EXTICR4_OFFSET    0x14  // External interrupt configuration register 4 (EXTI12–15)

//#define SYSCFG ((SYSCFG_TypeDef *) SYSCFG_BASE)




/* EXTI Group */
//------------------------------------------------
#define EXTI_BASE 0x40013C00

#define EXTI_IMR_OFFSET 0x00
#define EXTI_RTSR_OFFSET 0x08
#define EXTI_FTSR_OFFSET 0x0C
#define EXTI_PR_OFFSET 0x14




/* SPI Group */
//------------------------------------------------
#define SPI_BASE 0x40015000

#define SPI_CR1_OFFSET 0x00
#define SPI_CR2_OFFSET 0x04
#define SPI_SR_OFFSET 0x08
#define SPI_DR_OFFSET 0x0C




/* LTDC Group */
//------------------------------------------------
#define LTDC_BASE 0x40016800

#define LTDC_SSCR_OFFSET  0x08
#define LTDC_BPCR_OFFSET  0x0C
#define LTDC_AWCR_OFFSET  0x10
#define LTDC_TWCR_OFFSET  0x14
#define LTDC_GCR_OFFSET   0x18
#define LTDC_SRCR_OFFSET  0x24
#define LTDC_BCCR_OFFSET  0x2C
#define LTDC_LXCR_OFFSET(layerx)    (0x84 + 0x80 * (layerx - 1))
#define LTDC_LXWHPCR_OFFSET(layerx) (0x88 + 0x80 * (layerx - 1))
#define LTDC_LXWVPCR_OFFSET(layerx) (0x8C + 0x80 * (layerx - 1))
#define LTDC_LXPFCR_OFFSET(layerx)  (0x94 + 0x80 * (layerx - 1))

#define LTDC_LXCACR_OFFSET(layerx)  (0x98 + 0x80 * (layerx - 1))
#define LTDC_LXDCCR_OFFSET(layerx)  (0x9C + 0x80 * (layerx - 1))
#define LTDC_LXBFCR_OFFSET(layerx)  (0xA0 + 0x80 * (layerx - 1))

#define LTDC_LXCFBAR_OFFSET(layerx) (0xAC + 0x80 * (layerx - 1))
#define LTDC_LXCFBLR_OFFSET(layerx) (0xB0 + 0x80 * (layerx - 1))
#define LTDC_LXCFBLNR_OFFSET(layerx)(0xB4 + 0x80 * (layerx -1))


/* FMC Group */
//------------------------------------------------
#define FMC_BASE 0xA0000000

#define SDRAM_SDCR_OFFSET(bank)  (0x140 + ((bank - 1) * 4))
#define SDRAM_SDTR_OFFSET(bank)  (0x148 + ((bank - 1) * 4))
#define SDRAM_SDCMR_OFFSET  0x150
#define SDRAM_SDRTR_OFFSET  0x150

#endif /* MEM_MAP_H_ */
