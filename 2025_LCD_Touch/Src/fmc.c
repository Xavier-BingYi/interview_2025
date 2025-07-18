/*
 * sdram.c
 *
 *  Created on: Jul 5, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <usart.h>
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
	// Initialize all FMC-related GPIOs for SDRAM (AF12)
	fmc_gpio_init();

	// Enable AHB3 peripheral clock for FMC module
	rcc_enable_ahb3_clock();

	// 0. Configure SDCR2
	sdram_sdcr_write_field(2, SDRAM_SDCR_NC,    0x00); // Column address = 8 bits
	sdram_sdcr_write_field(2, SDRAM_SDCR_NR,    0x01); // Row address = 12 bits
	sdram_sdcr_write_field(2, SDRAM_SDCR_MWID,  0x01); // 16-bit data width
	sdram_sdcr_write_field(2, SDRAM_SDCR_NB,    0x01); // 4 internal banks


	// 1. Configure SDRAM control parameters (SDCLK, burst, pipe delay)
	sdram_sdcr_write_field(1, SDRAM_SDCR_SDCLK,  0x02); // SDCLK = HCLK/2
	sdram_sdcr_write_field(1, SDRAM_SDCR_RBURST, 0x01); // Enable burst read
	sdram_sdcr_write_field(1, SDRAM_SDCR_RPIPE,  0x00); // No read pipe delay

	// 2. Configure SDRAM timing parameters (TRCD, TRP, TRC)
	sdram_sdtr_write_field(1, SDRAM_SDTR_TRCD, 2); // RAS to CAS delay
	sdram_sdtr_write_field(1, SDRAM_SDTR_TRP,  2); // Row precharge delay
	sdram_sdtr_write_field(1, SDRAM_SDTR_TRC,  7); // Row cycle delay

	// 3. Send Clock Enable command (MODE = 001)
	sdram_sdcmr_write_field(SDRAM_SDCMR_MODE, 0x1); // MODE: Clock config enable
	sdram_sdcmr_write_field(SDRAM_SDCMR_CTB2, 0x1);  // Target Bank2

	// 4. Wait for SDRAM power-up delay (≥100us)
	delay_us(1000); // Wait 1 ms (safe margin)

	// 5. Send Precharge All command (MODE = 010)
	sdram_sdcmr_write_field(SDRAM_SDCMR_MODE, 0x2); // MODE: Precharge All
	sdram_sdcmr_write_field(SDRAM_SDCMR_CTB2, 0x1);  // Target Bank2

	// 6. Send Auto-refresh command (MODE = 011, NRFS = 8)
	sdram_sdcmr_write_field(SDRAM_SDCMR_MODE, 0x3); // MODE: Auto-refresh
	sdram_sdcmr_write_field(SDRAM_SDCMR_CTB2, 0x1);  // Target Bank2
	sdram_sdcmr_write_field(SDRAM_SDCMR_NRFS, 0x8);  // 8 consecutive refresh

	// 7. Load Mode Register command (MODE = 100, MRD = 0x0200)
	sdram_sdcmr_write_field(SDRAM_SDCMR_MODE, 0x4);   // MODE: Load mode register
	sdram_sdcmr_write_field(SDRAM_SDCMR_CTB2,  0x1);   // Target Bank2
	sdram_sdcmr_write_field(SDRAM_SDCMR_MRD,   0x0200); // Mode Register value

	// 8. Set SDRAM refresh rate: COUNT = 683
	sdram_sdrtr_write_field(SDRAM_SDRTR_COUNT, 683);
}

void sdram_self_test(void) {
    volatile uint32_t *sdram = (uint32_t*)0xD0000000; // SDRAM base address (Bank2)
    usart_printf("1");
    uint32_t test_val = 0xA5A5A5A5;
    usart_printf("2");
    // Write
    sdram[0] = test_val;
    usart_printf("3");
    // Read back and verify
    if (sdram[0] == test_val) {
    	usart_printf("SDRAM test passed!\n");
    } else {
    	usart_printf("SDRAM test FAILED! Read: %x \n", sdram[0]);
    }
    usart_printf("4");
}

void sdram_sdcr_write_field(uint8_t bank, sdram_sdcr_field_t field, uint32_t value){
    uint32_t addr = FMC_BASE + SDRAM_SDCR_OFFSET(bank);
    uint8_t shift = 0;
    uint8_t width = 0;

    switch(field) {
        case SDRAM_SDCR_NC:     shift = 0;  width = 2; break;
        case SDRAM_SDCR_NR:     shift = 2;  width = 2; break;
        case SDRAM_SDCR_MWID:   shift = 4;  width = 2; break;
        case SDRAM_SDCR_NB:     shift = 6;  width = 1; break;
        case SDRAM_SDCR_CAS:    shift = 7;  width = 2; break;
        case SDRAM_SDCR_WP:     shift = 9;  width = 1; break;
        case SDRAM_SDCR_SDCLK:  shift = 10; width = 2; break;
        case SDRAM_SDCR_RBURST: shift = 12; width = 1; break;
        case SDRAM_SDCR_RPIPE:  shift = 13; width = 2; break;
        default: return;
    }
    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void sdram_sdtr_write_field(uint8_t bank, sdram_sdtr_field_t field, uint32_t value){
	uint32_t addr = FMC_BASE + SDRAM_SDTR_OFFSET(bank);
	uint32_t shift = 0x4U * field;
    uint32_t mask = 0x0FU << shift;
    uint32_t data = (uint32_t)value << shift;
    io_writeMask(addr, data, mask);
}

void sdram_sdcmr_write_field(sdram_sdcmr_field_t field, uint32_t value){
	uint32_t addr = FMC_BASE + SDRAM_SDCMR_OFFSET;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch(field) {
        case SDRAM_SDCMR_MODE: shift = 0;  width = 3; break;
        case SDRAM_SDCMR_CTB2: shift = 3;  width = 1; break;
        case SDRAM_SDCMR_CTB1: shift = 4;  width = 1; break;
        case SDRAM_SDCMR_NRFS: shift = 5;  width = 4; break;
        case SDRAM_SDCMR_MRD:  shift = 9;  width = 13; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

void sdram_sdrtr_write_field(sdram_sdrtr_field_t field, uint32_t value) {
    uint32_t addr = FMC_BASE + SDRAM_SDRTR_OFFSET;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
    	case SDRAM_SDRTR_CRE:   shift = 0; width = 1; break;
        case SDRAM_SDRTR_COUNT: shift = 1; width = 13; break;
        case SDRAM_SDRTR_REIE:  shift = 14; width = 1; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((value & ((1U << width) - 1U)) << shift);
    io_writeMask(addr, data, mask);
}
