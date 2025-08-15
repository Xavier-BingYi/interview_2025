/*
 * RCC.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>
#include <rcc.h>

void rcc_cr_write_field(rcc_cr_field_t field, uint32_t value){
    uint32_t addr = RCC_BASE + RCC_CR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case RCC_CR_HSION:        shift = 0;   width = 1; break;
        case RCC_CR_HSIRDY:       shift = 1;   width = 1; break;
        case RCC_CR_HSITRIM:      shift = 3;   width = 5; break;
        case RCC_CR_HSICAL:       shift = 8;   width = 8; break;

        case RCC_CR_HSEON:        shift = 16;  width = 1; break;
        case RCC_CR_HSERDY:       shift = 17;  width = 1; break;
        case RCC_CR_HSEBYP:       shift = 18;  width = 1; break;
        case RCC_CR_CSSON:        shift = 19;  width = 1; break;

        case RCC_CR_PLLON:        shift = 24;  width = 1; break;
        case RCC_CR_PLLRDY:       shift = 25;  width = 1; break;

        case RCC_CR_PLLI2SON:     shift = 26;  width = 1; break;
        case RCC_CR_PLLI2SRDY:    shift = 27;  width = 1; break;

        case RCC_CR_PLLSAION:     shift = 28;  width = 1; break;
        case RCC_CR_PLLSAIRDY:    shift = 29;  width = 1; break;

        default: return;
    }
    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void rcc_cfgr_write_field(rcc_cfgr_field_t field, uint32_t value){
    uint32_t addr = RCC_BASE + RCC_CFGR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch(field){
        case RCC_CFGR_SW:    shift=0;  width=2; break;
        case RCC_CFGR_SWS:   shift=2;  width=2; break; /* 通常讀用，但保留一致性 */
        case RCC_CFGR_HPRE:  shift=4;  width=4; break;
        case RCC_CFGR_PPRE1: shift=10; width=3; break;
        case RCC_CFGR_PPRE2: shift=13; width=3; break;
        default: return;
    }

    uint32_t mask = ((1U<<width)-1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U<<width)-1U)) << shift;
    io_writeMask(addr, data, mask);
}

uint32_t rcc_cfgr_read_field(rcc_cfgr_field_t field){
    uint32_t addr = RCC_BASE + RCC_CFGR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch(field){
        case RCC_CFGR_SW:    shift=0;  width=2; break;
        case RCC_CFGR_SWS:   shift=2;  width=2; break;
        case RCC_CFGR_HPRE:  shift=4;  width=4; break;
        case RCC_CFGR_PPRE1: shift=10; width=3; break;
        case RCC_CFGR_PPRE2: shift=13; width=3; break;
        default: return (uint32_t)-1;
    }
    return (io_read(addr) >> shift) & ((1U<<width)-1U);
}

uint32_t rcc_cr_read_field(rcc_cr_field_t field){
    uint32_t addr = RCC_BASE + RCC_CR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case RCC_CR_HSION:        shift = 0;   width = 1; break;
        case RCC_CR_HSIRDY:       shift = 1;   width = 1; break;
        case RCC_CR_HSITRIM:      shift = 3;   width = 5; break;
        case RCC_CR_HSICAL:       shift = 8;   width = 8; break;

        case RCC_CR_HSEON:        shift = 16;  width = 1; break;
        case RCC_CR_HSERDY:       shift = 17;  width = 1; break;
        case RCC_CR_HSEBYP:       shift = 18;  width = 1; break;
        case RCC_CR_CSSON:        shift = 19;  width = 1; break;

        case RCC_CR_PLLON:        shift = 24;  width = 1; break;
        case RCC_CR_PLLRDY:       shift = 25;  width = 1; break;

        case RCC_CR_PLLI2SON:     shift = 26;  width = 1; break;
        case RCC_CR_PLLI2SRDY:    shift = 27;  width = 1; break;

        case RCC_CR_PLLSAION:     shift = 28;  width = 1; break;
        case RCC_CR_PLLSAIRDY:    shift = 29;  width = 1; break;

        default: return -1;
    }

    uint32_t field_data = (io_read(addr) >> shift) & ((1U << width) - 1U);

    return field_data;
}

void rcc_pllcfgr_write_field(rcc_pllcfgr_field_t field, uint32_t value){
    uint32_t addr = RCC_BASE + RCC_PLLCFGR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case RCC_PLLCFGR_PLLM:    shift = 0;   width = 6; break;  // Bits 5:0
        case RCC_PLLCFGR_PLLN:    shift = 6;   width = 9; break;  // Bits 14:6
        case RCC_PLLCFGR_PLLP:    shift = 16;  width = 2; break;  // Bits 17:16
        case RCC_PLLCFGR_PLLSRC:  shift = 22;  width = 1; break;  // Bit 22
        case RCC_PLLCFGR_PLLQ:    shift = 24;  width = 4; break;  // Bits 27:24
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void rcc_pllsaicfgr_write_field(rcc_pllsaicfgr_field_t field, uint32_t value){
    uint32_t addr = RCC_BASE + RCC_PLLSAICFGR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case RCC_PLLSAICFGR_PLLSAIN:   shift = 6;   width = 9; break;  // Bits 14:6
        case RCC_PLLSAICFGR_PLLSAIQ:   shift = 24;  width = 4; break;  // Bits 27:24
        case RCC_PLLSAICFGR_PLLSAIR:   shift = 28;  width = 3; break;  // Bits 30:28
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void rcc_dckcfgr_write_field(rcc_dckcfgr_field_t field, uint32_t value){
    uint32_t addr = RCC_BASE + RCC_DCKCFGR;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case RCC_DCKCFGR_PLLI2SDIVQ:   shift = 0;   width = 5; break;  // Bits 4:0
        case RCC_DCKCFGR_PLLSAIDIVQ:   shift = 8;   width = 5; break;  // Bits 12:8
        case RCC_DCKCFGR_PLLSAIDIVR:   shift = 16;  width = 2; break;  // Bits 17:16
        case RCC_DCKCFGR_SAI1ASRC:     shift = 20;  width = 2; break;  // Bits 21:20
        case RCC_DCKCFGR_SAI1BSRC:     shift = 22;  width = 2; break;  // Bits 23:22
        case RCC_DCKCFGR_TIMPRE:       shift = 24;  width = 1; break;  // Bit 24

        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void rcc_enable_ahb1_clock(uint8_t bit_pos){
	uint32_t addr = RCC_BASE + RCC_AHB1ENR;
	uint32_t bitmask = 1U << bit_pos;
	io_writeMask(addr, bitmask, bitmask);
}

void rcc_enable_ahb3_clock(void){
    uint32_t addr = RCC_BASE + RCC_AHB3ENR;
    io_writeMask(addr, RCC_AHB3ENR_FMCEN, RCC_AHB3ENR_FMCEN);
}

void rcc_enable_apb1_clock(uint8_t bit_pos){
	uint32_t addr = RCC_BASE + RCC_APB1ENR;
	uint32_t bitmask = 1U << bit_pos;
	io_writeMask(addr, bitmask, bitmask);
}

void rcc_enable_apb2_clock(uint8_t bit_pos){
	uint32_t addr = RCC_BASE + RCC_APB2ENR;
	uint32_t bitmask = 1U << bit_pos;
	io_writeMask(addr, bitmask, bitmask);
}

void rcc_restart_apb2_peripheral(uint8_t bit_pos) {
    uint32_t addr = RCC_BASE + RCC_APB2RSTR;
    uint32_t bitmask = 1U << bit_pos;
    io_writeMask(addr, bitmask, bitmask);
    io_writeMask(addr, 0,       bitmask);
}
