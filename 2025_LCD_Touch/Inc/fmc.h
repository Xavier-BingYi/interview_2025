/*
 * sdram.h
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */

#ifndef FMC_H_
#define FMC_H_

typedef enum{
    FMC_SDCR_NC = 0,
    FMC_SDCR_NR,
    FMC_SDCR_MWID,
    FMC_SDCR_NB,
    FMC_SDCR_CAS,
    FMC_SDCR_WP,
    FMC_SDCR_SDCLK,
    FMC_SDCR_RBURST,
    FMC_SDCR_RPIPE
} fmc_sdcr_field_t;

typedef enum {
    FMC_SDTR_TMRD = 0,   // [3:0]
    FMC_SDTR_TXSR,       // [7:4]
    FMC_SDTR_TRAS,       // [11:8]
    FMC_SDTR_TRC,        // [15:12]
    FMC_SDTR_TWR,        // [19:16]
    FMC_SDTR_TRP,        // [23:20]
    FMC_SDTR_TRCD        // [27:24]
} fmc_sdtr_field_t;

void fmc_gpio_init(void);
void fmc_init(void);
void fmc_sdcr_write_field(uint8_t bank, fmc_sdcr_field_t field, uint8_t value);
void fmc_sdtr_write_field(uint8_t bank, fmc_sdtr_field_t field, uint8_t value);


#endif /* FMC_H_ */
