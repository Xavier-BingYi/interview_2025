/*
 * mem_map.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Xavier
 */

#ifndef MEM_MAP_H_
#define MEM_MAP_H_


#define AHB1PERIPH_BASE 0x40020000
#define APB2PERIPH_BASE 0x40010000
#define APB1PERIPH_BASE 0x40000000

/* USART bus frequency（MCU default value）*/
#define FCK_APB1 4000000U  // USART2/3/4/5/7/8
#define FCK_APB2 8000000U  // USART1/6



#endif /* MEM_MAP_H_ */
