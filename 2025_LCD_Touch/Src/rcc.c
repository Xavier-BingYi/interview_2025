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
