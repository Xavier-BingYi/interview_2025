/*
 * RCC.c
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <mem_io.h>
#include <rcc.h>

void rcc_enable_ahb1_clock(uint8_t bit_pos){
	uint32_t addr = RCC_BASE + RCC_AHB1ENR;
	uint32_t bitmask = 1U << bit_pos;
	io_writeMask(addr, bitmask, bitmask);
}
