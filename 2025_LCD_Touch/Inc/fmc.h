/*
 * sdram.h
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */

#ifndef FMC_H_
#define FMC_H_

typedef enum{
    SDRAM_SDCR_NC = 0,
	SDRAM_SDCR_NR,
	SDRAM_SDCR_MWID,
	SDRAM_SDCR_NB,
	SDRAM_SDCR_CAS,
	SDRAM_SDCR_WP,
	SDRAM_SDCR_SDCLK,
	SDRAM_SDCR_RBURST,
	SDRAM_SDCR_RPIPE
} sdram_sdcr_field_t;

typedef enum {
	SDRAM_SDTR_TMRD = 0,   // [3:0]
	SDRAM_SDTR_TXSR,       // [7:4]
	SDRAM_SDTR_TRAS,       // [11:8]
	SDRAM_SDTR_TRC,        // [15:12]
	SDRAM_SDTR_TWR,        // [19:16]
	SDRAM_SDTR_TRP,        // [23:20]
	SDRAM_SDTR_TRCD        // [27:24]
} sdram_sdtr_field_t;

typedef enum {
	SDRAM_SDCMR_MODE = 0,
	SDRAM_SDCMR_CTB2,
	SDRAM_SDCMR_CTB1,
	SDRAM_SDCMR_NRFS,
	SDRAM_SDCMR_MRD,
} sdram_sdcmr_field_t;

typedef enum {
    SDRAM_SDRTR_CRE,     // Bit 0
    SDRAM_SDRTR_COUNT,  // Bits [13:1]
    SDRAM_SDRTR_REIE   // Bit 14
} sdram_sdrtr_field_t;

void fmc_gpio_init(void);
void fmc_init(void);
void sdram_sdcr_write_field(uint8_t bank, sdram_sdcr_field_t field, uint32_t value);
void sdram_sdtr_write_field(uint8_t bank, sdram_sdtr_field_t field, uint32_t value);
void sdram_sdcmr_write_field(sdram_sdcmr_field_t field, uint32_t value);
void sdram_sdrtr_write_field(sdram_sdrtr_field_t field, uint32_t value);

void sdram_self_test(void);

#endif /* FMC_H_ */
