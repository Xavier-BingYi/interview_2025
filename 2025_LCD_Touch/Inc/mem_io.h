/*
 * mem_io.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Xavier
 */

#ifndef MEM_IO_H_
#define MEM_IO_H_

void io_write(register uint32_t addr, register uint32_t val);
uint32_t io_read(register uint32_t addr);
void io_writeMask(uint32_t addr, uint32_t data, uint32_t mask);

#endif /* MEM_IO_H_ */
