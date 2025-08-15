/*
 * clock.h
 *
 *  Created on: Aug 14, 2025
 *      Author: Xavier
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>

/* backfill base/offset (若你已有則會被忽略) */
#ifndef PWR_BASE
#define PWR_BASE (APB1PERIPH_BASE + 0x7000U) /* 0x40007000 */
#endif
#ifndef PWR_CR_OFFSET
#define PWR_CR_OFFSET  0x00
#endif
#ifndef PWR_CSR_OFFSET
#define PWR_CSR_OFFSET 0x04
#endif

typedef enum {
    PWR_CR_VOS   = 14, /* width=2 */
    PWR_CR_ODEN  = 16, /* width=1 */
    PWR_CR_ODSWEN= 17  /* width=1 */
} pwr_cr_field_t;

typedef enum {
    PWR_CSR_ODRDY = 16 /* width=1 */
} pwr_csr_field_t;







#ifndef FLASH_BASE
#define FLASH_BASE 0x40023C00UL
#endif
#ifndef FLASH_ACR_OFFSET
#define FLASH_ACR_OFFSET 0x00
#endif

typedef enum {
    FLASH_ACR_LATENCY = 0 /* width=4 (保留 0xF 面罩習慣) */
} flash_acr_field_t;




void system_clock_setup(void);



#endif /* CLOCK_H_ */
