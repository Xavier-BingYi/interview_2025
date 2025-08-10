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


typedef enum {
    LTDC_CR_HSION      = 0,   // Bit 0: HSI oscillator ON
    LTDC_CR_HSIRDY     = 1,   // Bit 1: HSI oscillator ready
    LTDC_CR_HSITRIM    = 3,   // Bits 7:3: HSI trimming (5-bit)
    LTDC_CR_HSICAL     = 8,   // Bits 15:8: HSI calibration (8-bit)

    LTDC_CR_HSEON      = 16,  // Bit 16: HSE oscillator ON
    LTDC_CR_HSERDY     = 17,  // Bit 17: HSE oscillator ready
    LTDC_CR_HSEBYP     = 18,  // Bit 18: HSE bypass
    LTDC_CR_CSSON      = 19,  // Bit 19: Clock security system enable

    LTDC_CR_PLLON      = 24,  // Bit 24: Main PLL enable
    LTDC_CR_PLLRDY     = 25,  // Bit 25: Main PLL ready

    LTDC_CR_PLLI2SON   = 26,  // Bit 26: PLLI2S enable
    LTDC_CR_PLLI2SRDY  = 27,  // Bit 27: PLLI2S ready

    LTDC_CR_PLLSAION   = 28,  // Bit 28: PLLSAI enable
    LTDC_CR_PLLSAIRDY  = 29   // Bit 29: PLLSAI ready
} ltdc_cr_field_t;

typedef enum {
    LTDC_PLLCFGR_PLLM    = 0,   // Bits 5:0: Main PLL and PLLI2S division factor
    LTDC_PLLCFGR_PLLN    = 6,   // Bits 14:6: Main PLL multiplication factor
    LTDC_PLLCFGR_PLLP    = 16,  // Bits 17:16: Main PLL division factor for SYSCLK
    LTDC_PLLCFGR_PLLSRC  = 22,  // Bit 22: Clock source for PLL and PLLI2S
    LTDC_PLLCFGR_PLLQ    = 24,  // Bits 27:24: PLL division for USB OTG FS, SDIO, RNG
} ltdc_pllcfgr_field_t;

typedef enum {
    LTDC_PLLSAICFGR_PLLSAIN = 6,   // Bits 14:6: PLLSAI multiplier for VCO
    LTDC_PLLSAICFGR_PLLSAIQ = 24,  // Bits 27:24: PLLSAI_Q division
    LTDC_PLLSAICFGR_PLLSAIR = 28   // Bits 30:28: PLLSAI_R division (LCD)
} ltdc_pllsaicfgr_field_t;

typedef enum {
    LTDC_DCKCFGR_PLLI2SDIVQ   = 0,   // Bits 4:0: PLLI2S_Q division
    LTDC_DCKCFGR_PLLSAIDIVQ   = 8,   // Bits 12:8: PLLSAI_Q division
    LTDC_DCKCFGR_PLLSAIDIVR   = 16,  // Bits 17:16: PLLSAI_R division (LCD)
    LTDC_DCKCFGR_SAI1ASRC     = 20,  // Bits 21:20: SAI1-A clock source
    LTDC_DCKCFGR_SAI1BSRC     = 22,  // Bits 23:22: SAI1-B clock source
    LTDC_DCKCFGR_TIMPRE       = 24   // Bit 24: Timer clock prescaler
} ltdc_dckcfgr_field_t;

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



void rcc_cr_write_field(ltdc_cr_field_t field, uint32_t value);
uint32_t rcc_cr_read_field(ltdc_cr_field_t field);
void rcc_pllcfgr_write_field(ltdc_pllcfgr_field_t field, uint32_t value);
void rcc_pllsaicfgr_write_field(ltdc_pllsaicfgr_field_t field, uint32_t value);
void rcc_dckcfgr_write_field(ltdc_dckcfgr_field_t field, uint32_t value);

/* RCC APB2 bit positions */
#define RCC_APB2EN_SYSCFG 14  // SYSCFG EN bit = bit 14
#define RCC_APB2EN_SPI5 20  // SPI5 EN bit = bit 20
#define RCC_APB2EN_LTDC 26  // LTDC EN bit = bit 26

/* AHB3 peripheral clock enable bit for FMC controller */
#define RCC_AHB3ENR_FMCEN (1U << 0)


void rcc_enable_ahb1_clock(uint8_t bit_pos);
void rcc_enable_ahb3_clock(void);
void rcc_enable_apb1_clock(uint8_t bit_pos);
void rcc_enable_apb2_clock(uint8_t bit_pos);


#endif /* RCC_H_ */
