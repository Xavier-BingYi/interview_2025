/*
 * clock.c
 *
 *  Created on: Aug 14, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>
#include <rcc.h>
#include <clock.h>

static inline void pwr_cr_write_field(pwr_cr_field_t field, uint32_t value){
    uint32_t addr = PWR_BASE + PWR_CR_OFFSET;
    uint8_t shift = (uint8_t)field;
    uint8_t width = (field==PWR_CR_VOS)?2:1;
    uint32_t mask = ((1U<<width)-1U) << shift;
    uint32_t data = (value << shift) & mask;
    io_writeMask(addr, data, mask);
}

static inline uint32_t pwr_csr_read_field(pwr_csr_field_t field){
    uint32_t addr = PWR_BASE + PWR_CSR_OFFSET;
    uint8_t shift = (uint8_t)field;
    return (io_read(addr) >> shift) & 0x1U;
}






static inline void flash_acr_write_field(flash_acr_field_t field, uint32_t value){
    (void)field;
    uint32_t addr = FLASH_BASE + FLASH_ACR_OFFSET;
    uint8_t shift=0, width=4;
    uint32_t mask = ((1U<<width)-1U) << shift;
    uint32_t data = (value << shift) & mask;
    io_writeMask(addr, data, mask);
}





void system_clock_setup(void)
{
    /* 1) Flash wait states = 5 */
    flash_acr_write_field(FLASH_ACR_LATENCY, 0x5U);

    /* 2) PWR clock + OverDrive */
    rcc_enable_apb1_clock(RCC_APB1EN_PWREN);       /* 啟用 PWR 模組時鐘 */
    pwr_cr_write_field(PWR_CR_VOS, 0x3U);
    pwr_cr_write_field(PWR_CR_ODEN, 1U);
    while (pwr_csr_read_field(PWR_CSR_ODRDY)==0U) { }
    pwr_cr_write_field(PWR_CR_ODSWEN, 1U);

    /* 3) 主 PLL: M=8, N=180, P=0b00(÷2) */
    rcc_pllcfgr_write_field(RCC_PLLCFGR_PLLSRC, 0);            // PLLSRC=HSI
    rcc_pllcfgr_write_field(RCC_PLLCFGR_PLLM, 0x08U);
    rcc_pllcfgr_write_field(RCC_PLLCFGR_PLLN, 180U);
    rcc_pllcfgr_write_field(RCC_PLLCFGR_PLLP, 0x0U);


    /* 5) AHB/APB 分頻：AHB=/1, APB1=/4, APB2=/2 */
    rcc_cfgr_write_field(RCC_CFGR_HPRE,  0x0U); // AHB = /1
    rcc_cfgr_write_field(RCC_CFGR_PPRE1, 0x5U); // APB1 = /4
    rcc_cfgr_write_field(RCC_CFGR_PPRE2, 0x4U); // APB2 = /2

    /* 6) 啟動主 PLL 並等待 */
    rcc_cr_write_field(RCC_CR_PLLON, 1U);
    while (rcc_cr_read_field(RCC_CR_PLLRDY)==0U) { }

    /* 7) 切換 SYSCLK=PLL 並確認 */
    rcc_cfgr_write_field(RCC_CFGR_SW, 0x2U);
    while (rcc_cfgr_read_field(RCC_CFGR_SWS) != 0x2U) { }
}
