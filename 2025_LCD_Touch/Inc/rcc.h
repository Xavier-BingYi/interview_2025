/*
 * RCC.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef RCC_H_
#define RCC_H_

/* USART bus frequency（MCU default value）*/
#define FCK_APB2 90000000U  // USART1/6

/* I2C bus frequency（MCU default value）*/
#define FCK_APB1 45000000U  // I2C3

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
    RCC_CR_HSION      = 0,   // Bit 0: HSI oscillator ON
    RCC_CR_HSIRDY     = 1,   // Bit 1: HSI oscillator ready
    RCC_CR_HSITRIM    = 3,   // Bits 7:3: HSI trimming (5-bit)
    RCC_CR_HSICAL     = 8,   // Bits 15:8: HSI calibration (8-bit)

    RCC_CR_HSEON      = 16,  // Bit 16: HSE oscillator ON
    RCC_CR_HSERDY     = 17,  // Bit 17: HSE oscillator ready
    RCC_CR_HSEBYP     = 18,  // Bit 18: HSE bypass
    RCC_CR_CSSON      = 19,  // Bit 19: Clock security system enable

    RCC_CR_PLLON      = 24,  // Bit 24: Main PLL enable
    RCC_CR_PLLRDY     = 25,  // Bit 25: Main PLL ready

    RCC_CR_PLLI2SON   = 26,  // Bit 26: PLLI2S enable
    RCC_CR_PLLI2SRDY  = 27,  // Bit 27: PLLI2S ready

    RCC_CR_PLLSAION   = 28,  // Bit 28: PLLSAI enable
    RCC_CR_PLLSAIRDY  = 29   // Bit 29: PLLSAI ready
} rcc_cr_field_t;

typedef enum {
    RCC_CFGR_SW   = 0,
    RCC_CFGR_SWS  = 2,
    RCC_CFGR_HPRE = 4,
    RCC_CFGR_PPRE1= 10,
    RCC_CFGR_PPRE2= 13
} rcc_cfgr_field_t;

typedef enum {
    RCC_PLLCFGR_PLLM    = 0,   // Bits 5:0: Main PLL and PLLI2S division factor
    RCC_PLLCFGR_PLLN    = 6,   // Bits 14:6: Main PLL multiplication factor
    RCC_PLLCFGR_PLLP    = 16,  // Bits 17:16: Main PLL division factor for SYSCLK
    RCC_PLLCFGR_PLLSRC  = 22,  // Bit 22: Clock source for PLL and PLLI2S
    RCC_PLLCFGR_PLLQ    = 24,  // Bits 27:24: PLL division for USB OTG FS, SDIO, RNG
} rcc_pllcfgr_field_t;

typedef enum {
    RCC_PLLSAICFGR_PLLSAIN = 6,   // Bits 14:6: PLLSAI multiplier for VCO
    RCC_PLLSAICFGR_PLLSAIQ = 24,  // Bits 27:24: PLLSAI_Q division
    RCC_PLLSAICFGR_PLLSAIR = 28   // Bits 30:28: PLLSAI_R division (LCD)
} rcc_pllsaicfgr_field_t;

typedef enum {
    RCC_DCKCFGR_PLLI2SDIVQ   = 0,   // Bits 4:0: PLLI2S_Q division
    RCC_DCKCFGR_PLLSAIDIVQ   = 8,   // Bits 12:8: PLLSAI_Q division
    RCC_DCKCFGR_PLLSAIDIVR   = 16,  // Bits 17:16: PLLSAI_R division (LCD)
    RCC_DCKCFGR_SAI1ASRC     = 20,  // Bits 21:20: SAI1-A clock source
    RCC_DCKCFGR_SAI1BSRC     = 22,  // Bits 23:22: SAI1-B clock source
    RCC_DCKCFGR_TIMPRE       = 24   // Bit 24: Timer clock prescaler
} rcc_dckcfgr_field_t;

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



void rcc_cr_write_field(rcc_cr_field_t field, uint32_t value);
uint32_t rcc_cr_read_field(rcc_cr_field_t field);
void rcc_cfgr_write_field(rcc_cfgr_field_t field, uint32_t value);
uint32_t rcc_cfgr_read_field(rcc_cfgr_field_t field);
void rcc_pllcfgr_write_field(rcc_pllcfgr_field_t field, uint32_t value);
void rcc_pllsaicfgr_write_field(rcc_pllsaicfgr_field_t field, uint32_t value);
void rcc_dckcfgr_write_field(rcc_dckcfgr_field_t field, uint32_t value);


/* RCC APB1 enable bit */
#define RCC_APB1EN_TIM2EN 0   // TIM2
#define RCC_APB1EN_TIM7EN 5   // TIM7
#define RCC_APB1EN_I2C3EN 23  // I2C3
#define RCC_APB1EN_PWREN  28  // PWR

/* RCC APB2 enable bit */
#define RCC_APB2EN_SYSCFG 14  // SYSCFG
#define RCC_APB2EN_SPI5   20  // SPI5
#define RCC_APB2EN_LTDC   26  // LTDC

/* RCC APB2 reset bit */
#define RCC_APB2RSTR_LTDC 26  // LTDC



/* AHB3 peripheral clock enable bit for FMC controller */
#define RCC_AHB3ENR_FMCEN (1U << 0)


void rcc_enable_ahb1_clock(uint8_t bit_pos);
void rcc_enable_ahb3_clock(void);
void rcc_enable_apb1_clock(uint8_t bit_pos);
void rcc_enable_apb2_clock(uint8_t bit_pos);


void rcc_restart_apb2_peripheral(uint8_t bit_pos);


#endif /* RCC_H_ */
