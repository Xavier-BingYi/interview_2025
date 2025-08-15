/*
 * mem_io.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef MEM_IO_H_
#define MEM_IO_H_

void delay_us(uint32_t us);

void io_write(register uint32_t addr, register uint32_t val);
uint32_t io_read(register uint32_t addr);
void io_writeMask(uint32_t addr, uint32_t data, uint32_t mask);
void io_write8(uint32_t addr, uint8_t val);
uint8_t io_read8(uint32_t addr);

#endif /* MEM_IO_H_ */
