/*
 * sdram.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <mem_io.h>
#include <fmc.h>

void fmc_gpio_init(void)
{
    // Enable GPIO clocks
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOB);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOE);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

    // Address lines A0~A11
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // A0
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // A1
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_1, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_2, GPIO_MODE_ALTERNATE);  // A2
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_2, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // A3
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_3, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);  // A4
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_4, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_5, GPIO_MODE_ALTERNATE);  // A5
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_5, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // A6
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_12, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_13, GPIO_MODE_ALTERNATE); // A7
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_13, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // A8
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // A9
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // A10
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // A11
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_1, ALTERNATE_AF12);

    // Data lines D0~D15
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // D0
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // D1
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // D2
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // D3
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_1, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // D4
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_7, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // D5
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // D6
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_9, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // D7
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_10, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // D8
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_11, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // D9
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_12, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_13, GPIO_MODE_ALTERNATE); // D10
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_13, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // D11
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // D12
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // D13
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // D14
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_9, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // D15
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_10, ALTERNATE_AF12);

    // Control signals
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // SDCLK
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOB_BASE, GPIO_PIN_5, GPIO_MODE_ALTERNATE);  // SDCKE1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_5, ALTERNATE_AF12);
    gpio_set_mode(GPIOB_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // SDNE1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_6, ALTERNATE_AF12);
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // SDNWE
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // SDNRAS
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_11, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // SDNCAS
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_15, ALTERNATE_AF12);
}

void fmc_init(void){
	rcc_enable_ahb3_clock();

	// Configure FMC_SDCR
    fmc_sdcr_write_field(1, FMC_SDCR_NC,    0x01); // Column = 9 bits
    fmc_sdcr_write_field(1, FMC_SDCR_NR,    0x01); // Row = 12 bits
    fmc_sdcr_write_field(1, FMC_SDCR_MWID,  0x01); // Memory width = 16-bit
    fmc_sdcr_write_field(1, FMC_SDCR_NB,    0x01); // Bank number = 4 banks
    fmc_sdcr_write_field(1, FMC_SDCR_CAS,   0x02); // CAS latency = 2 cycles
    fmc_sdcr_write_field(1, FMC_SDCR_WP,    0x00); // Write protect disable
    fmc_sdcr_write_field(1, FMC_SDCR_SDCLK, 0x02); // SDRAM clock = HCLK/2
    fmc_sdcr_write_field(1, FMC_SDCR_RBURST,0x01); // Enable burst read
    fmc_sdcr_write_field(1, FMC_SDCR_RPIPE, 0x00); // Read pipe delay = 0

    // Configure FMC_SDTR
    fmc_sdtr_write_field(1, FMC_SDTR_TMRD,  2); // Load to Active delay
    fmc_sdtr_write_field(1, FMC_SDTR_TXSR,  7); // Exit self-refresh delay
    fmc_sdtr_write_field(1, FMC_SDTR_TRAS,  4); // Self refresh time
    fmc_sdtr_write_field(1, FMC_SDTR_TRC,   7); // Row cycle delay
    fmc_sdtr_write_field(1, FMC_SDTR_TWR,   2); // Write recovery time
    fmc_sdtr_write_field(1, FMC_SDTR_TRP,   2); // Row precharge delay
    fmc_sdtr_write_field(1, FMC_SDTR_TRCD,  2); // Row to column delay
}

void fmc_sdcr_write_field(uint8_t bank, fmc_sdcr_field_t field, uint8_t value){
    uint8_t shift = 0;
    uint8_t width = 0;

    switch(field) {
        case FMC_SDCR_NC:     shift = 0;  width = 2; break;
        case FMC_SDCR_NR:     shift = 2;  width = 2; break;
        case FMC_SDCR_MWID:   shift = 4;  width = 2; break;
        case FMC_SDCR_NB:     shift = 6;  width = 1; break;
        case FMC_SDCR_CAS:    shift = 7;  width = 2; break;
        case FMC_SDCR_WP:     shift = 9;  width = 1; break;
        case FMC_SDCR_SDCLK:  shift = 10; width = 2; break;
        case FMC_SDCR_RBURST: shift = 12; width = 1; break;
        case FMC_SDCR_RPIPE:  shift = 13; width = 2; break;
        default: return;
    }
    uint32_t addr = FMC_BASE + FMC_SDCR_OFFSET(bank);
    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void fmc_sdtr_write_field(uint8_t bank, fmc_sdtr_field_t field, uint8_t value){
	uint32_t addr = FMC_BASE + FMC_SDTR_OFFSET(bank);
	uint32_t shift = 0x4U * field;
    uint32_t mask = 0x0FU << shift;
    uint32_t data = (uint32_t)value << shift;
    io_writeMask(addr, data, mask);
}
