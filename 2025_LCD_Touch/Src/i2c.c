/*
 * i2c.c
 *
 *  Created on: Aug 20, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>
#include <gpio.h>
#include <usart.h>
#include <rcc.h>
#include <i2c.h>

void i2c3_gpio_init(void) {
    // GPIOA / GPIOC clock
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);

    // PA8 -> I2C3_SCL (AF4, OD, High, no pull)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_8, ALTERNATE_AF4);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_8, GPIO_OTYPE_OPENDRAIN);
    gpio_set_pupdr(GPIOA_BASE, GPIO_PIN_8, GPIO_PUPD_NONE);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_8, GPIO_SPEED_HIGH);

    // PC9 -> I2C3_SDA (AF4, OD, High, no pull)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_9, ALTERNATE_AF4);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_9, GPIO_OTYPE_OPENDRAIN);
    gpio_set_pupdr(GPIOC_BASE, GPIO_PIN_9, GPIO_PUPD_NONE);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_9, GPIO_SPEED_HIGH);
}

void i2c_init(void) {
    const uint32_t PCLK1_MHZ  = 45u;        // APB1 peripheral clock in MHz (I2C input clock)
    const uint32_t I2C_SCL_HZ = 300000u;    // Target SCL frequency (Fast-mode 300 kHz)

    // TRISE: configure with maximum SCL rise time + 1 (Fast-mode uses 300 ns)
    // TRISE = (t_r_max / T_PCLK1) + 1 = 0.3 * PCLK1_MHz + 1
    const uint32_t I2C_TRISE_REG = 15u;     // TRISE register value for Fast-mode (APB1=45 MHz)

    // CCR formula (Fast-mode, duty=0): CCR = PCLK1 / (3 * Fscl)
    uint32_t ccr = (PCLK1_MHZ * 1000000u) / (3u * I2C_SCL_HZ);

    i2c3_gpio_init();

    // Enable I2C3 peripheral clock on APB1 bus
    rcc_enable_apb1_clock(RCC_APB1EN_I2C3EN);

    // Disable I2C before configuring timing registers
    i2c_cr1_write_field(I2C3_BASE, I2C_CR1_PE, 0);

    // CR2.FREQ must be set to APB1 clock in MHz (valid range: 2..50)
    i2c_cr2_write_field(I2C3_BASE, I2C_CR2_FREQ, PCLK1_MHZ);

    // Configure Fast-mode (FS=1), DUTY=0, CCR[11:0] = divider value
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_FS, 1);
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_DUTY, 0);
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_CCR, ccr);

    // Configure maximum rise time
    i2c_trise_write_field(I2C3_BASE, I2C_TRISE_REG);

    // Enable I2C peripheral
    i2c_cr1_write_field(I2C3_BASE, I2C_CR1_PE, 1);
}

void i2c_cr1_write_field(uint32_t i2c_base, i2c_cr1_field_t field, uint32_t value) {
    uint32_t addr = i2c_base + I2C_CR1_OFFSET;
    uint32_t mask = 1U << field;
    uint32_t data = value << field;
    io_writeMask(addr, data, mask);
}

void i2c_cr2_write_field(uint32_t i2c_base, i2c_cr2_field_t field, uint32_t value) {
    uint32_t addr = i2c_base + I2C_CR2_OFFSET;
    uint8_t width = 0;

    switch (field) {
		case I2C_CR2_FREQ:         width = 6; break;
		case I2C_CR2_ITERREN:      width = 1; break;
		case I2C_CR2_ITEVTEN:      width = 1; break;
        case I2C_CR2_ITBUFEN:      width = 1; break;
        case I2C_CR2_DMAEN:      width = 1; break;
        case I2C_CR2_LAST:      width = 1; break;
        default: return;
    }

    uint32_t mask = ((1u << width) - 1u) << field;
    uint32_t data = (value & ((1u << width) - 1u)) << field;
    io_writeMask(addr, data, mask);
}

void i2c_ccr_write_field(uint32_t i2c_base, i2c_ccr_field_t field, uint32_t value) {
    uint32_t addr = i2c_base + I2C_CCR_OFFSET;
    uint8_t width = 0;

    switch (field) {
		case I2C_CCR_CCR:     width = 12; break;
		case I2C_CCR_DUTY:    width = 1; break;
		case I2C_CCR_FS:      width = 1; break;
        default: return;
    }

    uint32_t mask = ((1u << width) - 1u) << field;
    uint32_t data = (value & ((1u << width) - 1u)) << field;
    io_writeMask(addr, data, mask);
}

void i2c_trise_write_field(uint32_t i2c_base, uint32_t value) {
    uint32_t addr = i2c_base + I2C_TRISE_OFFSET;
    io_write(addr, value);
}

void i2c_sr1_write_field(uint32_t i2c_base, i2c_sr1_field_t field, uint32_t value) {
    uint32_t addr = i2c_base + I2C_SR1_OFFSET;
    uint32_t mask = 1u << field;
    uint32_t data = value << field;
    io_writeMask(addr, data, mask);
}

uint8_t i2c_sr1_read_field(uint32_t i2c_base, i2c_sr1_field_t field) {
    uint32_t addr = i2c_base + I2C_SR1_OFFSET;
    return (io_read(addr) & (1U << (uint32_t)field)) ? 1 : 0;
}

uint8_t i2c_sr2_read_field(uint32_t i2c_base, i2c_sr2_field_t field) {
    uint32_t addr = i2c_base + I2C_SR2_OFFSET;
    return (io_read(addr) & (1U << (uint32_t)field)) ? 1 : 0;
}

int8_t i2c_master_write(uint32_t i2c_base, uint8_t slave_addr7, uint8_t reg, uint8_t data) {
	const int8_t I2C_ERR_NACK_ADDR  = -1;
	const int8_t I2C_ERR_NACK_DATA  = -2;

	// Ensure 7-bit address
    slave_addr7 &= 0x7Fu;

    // 1) Wait until bus is free
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    // 2) Generate START, wait EV5 (SB=1)
    i2c_cr1_write_field(i2c_base, I2C_CR1_START, 1);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_SB) == 0);

    // 3) Send slave address (write : LSB = 0)
    io_write(i2c_base + I2C_DR_OFFSET, (slave_addr7 << 1) | 0u);

    // 4) Wait EV6 (ADDR=1) then clear by SR1->SR2 read
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0); // clear flag
            return I2C_ERR_NACK_ADDR;
        }
    }
    (void)io_read(i2c_base + I2C_SR1_OFFSET);
    (void)io_read(i2c_base + I2C_SR2_OFFSET);

    // 5) Wait EV8_1 (TxE=1), write reg
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }
    io_write(i2c_base + I2C_DR_OFFSET, reg);

    // 6) Wait EV8 (TxE=1), write data
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }
    io_write(i2c_base + I2C_DR_OFFSET, data);

    // 7) Wait EV8_2 (TxE=1 && BTF=1) then STOP
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0); // clear flag
            return I2C_ERR_NACK_DATA;
        }
    }
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }

    // 8) Generate STOP
    i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);

    // 9) wait bus released (BUSY=0)
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    return 0;
}

int8_t i2c_master_read(uint32_t i2c_base, uint8_t slave_addr7,
                           uint8_t reg_addr, uint8_t *data, uint16_t len) {
    if (len == 0) return 0;
    slave_addr7 &= 0x7Fu;

    // 1) Wait until bus is free
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    // 2) Generate START, wait EV5 (SB=1)
    i2c_cr1_write_field(i2c_base, I2C_CR1_START, 1);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_SB) == 0);

    // 3) Send slave address (write mode, LSB=0)
    io_write(i2c_base + I2C_DR_OFFSET, (slave_addr7 << 1) | 0u);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0);
    (void)io_read(i2c_base + I2C_SR1_OFFSET);
    (void)io_read(i2c_base + I2C_SR2_OFFSET);

    // 4) Send register address
    io_write(i2c_base + I2C_DR_OFFSET, reg_addr);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);

    // 5) Generate repeated START
    i2c_cr1_write_field(i2c_base, I2C_CR1_START, 1);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_SB) == 0);

    // 6) Send slave address (read mode, LSB=1)
    io_write(i2c_base + I2C_DR_OFFSET, (slave_addr7 << 1) | 1u);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0);

    // 7) Read data (same handling as before)
    if (len == 1) {
        i2c_cr1_write_field(i2c_base, I2C_CR1_POS, 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_RXNE) == 0);
        data[0] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
    }
    else if (len == 2) {
        i2c_cr1_write_field(i2c_base, I2C_CR1_POS, 1);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
        data[0] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        data[1] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        i2c_cr1_write_field(i2c_base, I2C_CR1_POS, 0);
    }
    else {
        // More than two bytes
        i2c_cr1_write_field(i2c_base, I2C_CR1_POS, 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 1);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);

        uint16_t remaining = len;

        while (remaining > 3) {
            while (i2c_sr1_read_field(i2c_base, I2C_SR1_RXNE) == 0);
            *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
            remaining--;
        }

        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);

        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        remaining--;

        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);

        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
    }

    // wait until BUSY=0
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    return 0;
}

void i2c_touch_init(void)
{
    // 1) Enable clocks: GPIO/TSC/ADC ON, Temperature sensor clock OFF for power saving.
    uint8_t sys_ctrl2_value = SYS_TS_OFF; // 0x08: TS_OFF=1, GPIO_OFF=0, TSC_OFF=0, ADC_OFF=0
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, SYS_CTRL2_ADDR, sys_ctrl2_value);

	// 2) Program TSC operating mode and tracking index with EN=0.
    // TRACK = 010 (8); OP_MOD = 000 (X, Y, Z acquisition)
    uint8_t tsc_ctrl_value = (uint8_t)((0b010 << TSC_CTRL_TRACK_SHIFT) | (0b000 << TSC_CTRL_OPMOD_SHIFT)); // EN=0
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, TSC_CTRL_ADDR, tsc_ctrl_value);

    // 3-1) Configure INT pin behavior: active-low, edge-triggered; then enable global interrupt.
    uint8_t int_ctrl_value = (uint8_t)(INT_TYPE | GLOBAL_INT); // INT_POLARITY=0 (active low), edge, global enable
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, INT_CTRL_ADDR, int_ctrl_value);

    // 3-2) Enable touch-detect interrupt source; clear any pending status.
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, INT_EN_ADDR, INT_EN_TOUCH_DET);
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, INT_STA_ADDR, 0xFF); // write-1-to-clear all flags

    // 4) Configure TSC timing: 4-sample averaging, 100us touch-detect delay, 100us settling.
    uint8_t tsc_cfg_value =
        (uint8_t)((0b10  << TSC_CFG_AVE_SHIFT)  |   // AVE_CTRL = 4 samples
                  (0b010 << TSC_CFG_TDLY_SHIFT) |   // TOUCH_DET_DELAY = 100 us
                  (0b001));                         // SETTLING = 100 us
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, TSC_CFG_ADDR, tsc_cfg_value);

    // 5) Enable TSC after programming OP_MOD/TRACK.
    tsc_ctrl_value |= TSC_CTRL_EN; // set EN=1
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, TSC_CTRL_ADDR, tsc_ctrl_value);

}

TouchPoint i2c_touch_read_xyz(void) {
    uint8_t data[4];
    TouchPoint coord = {0};

    // Read 4 bytes (X,Y,Z packed)
    i2c_master_read(I2C3_BASE, SLAVE_ADDR7, TSC_DATA_XYZ_ADDR, data, 4);

    // Unpack data according to datasheet Table 16
    coord.x = ((uint16_t)data[0] << 4) | (data[1] & 0x0F);              // X[11:0]
    coord.y = ((uint16_t)(data[1] & 0xF0) << 4) | (uint16_t)data[2];    // Y[11:0]
    coord.z = data[3];                                                  // Z[7:0]

	return coord;
}
