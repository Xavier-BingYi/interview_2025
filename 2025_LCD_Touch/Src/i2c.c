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
#include <ltdc.h>

// Calibration range (measured raw min/max from your panel)
static const uint16_t CAL_X0 = 6;    // raw_x min (left side)
static const uint16_t CAL_X1 = 237;  // raw_x max (right side)
static const uint16_t CAL_Y0 = 4;    // raw_y min (top side)
static const uint16_t CAL_Y1 = 239;  // raw_y max (bottom side)

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

    // 5) Configure FIFO threshold: trigger interrupt when 1 sample is available
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, FIFO_TH_ADDR, 1);

    // 6) Reset FIFO once during init, ensure it's empty before enabling TSC
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, FIFO_STA_ADDR, FIFO_RESET);
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, FIFO_STA_ADDR, 0x00); // release reset

    // 7) Enable TSC after programming OP_MOD/TRACK.
    tsc_ctrl_value |= TSC_CTRL_EN; // set EN=1
    i2c_master_write(I2C3_BASE, SLAVE_ADDR7, TSC_CTRL_ADDR, tsc_ctrl_value);

}

// Tiny median for n <= 5
static uint16_t median_u16(uint16_t *a, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        uint8_t m = i;
        for (uint8_t j = i + 1; j < n; j++) {
            if (a[j] < a[m]) m = j;
        }
        uint16_t t = a[i]; a[i] = a[m]; a[m] = t;
    }
    return a[n / 2];
}

TouchPoint i2c_touch_read_xyz(void) {
    uint8_t data[5];
    TouchPoint coord = (TouchPoint){0};

    // Read one sample: [XH, XL, YH, YL, Z]
    i2c_master_read(I2C3_BASE, SLAVE_ADDR7, TSC_DATA_X, data, 5);

    // Collect up to 5 samples from FIFO for smoothing
	uint16_t xs[5], ys[5];
	uint8_t  zs[5];
	uint8_t  cnt = 0;

	xs[cnt] = ((uint16_t)data[0] << 4) | (data[1] & 0x0F);
	ys[cnt] = ((uint16_t)data[2] << 4) | (data[3] & 0x0F);
	zs[cnt] = data[4];
	cnt++;

    // Read remaining samples (at most 4 more) from FIFO
    uint8_t left = 0;
    i2c_master_read(I2C3_BASE, SLAVE_ADDR7, FIFO_SIZE_ADDR, &left, 1);
    while (left > 0 && cnt < 5) {
        uint8_t b[5];
        i2c_master_read(I2C3_BASE, SLAVE_ADDR7, TSC_DATA_X, b, 5);
        xs[cnt] = ((uint16_t)b[0] << 4) | (b[1] & 0x0F);
        ys[cnt] = ((uint16_t)b[2] << 4) | (b[3] & 0x0F);
        zs[cnt] = b[4];
        cnt++;
        left--;
    }

    // Median on X/Y for jitter reduction; Z take max (or average if you prefer)
    uint16_t raw_x = median_u16(xs, cnt);
    uint16_t raw_y = median_u16(ys, cnt);
    uint8_t  raw_z = 0;
    for (uint8_t i = 0; i < cnt; i++) if (zs[i] > raw_z) raw_z = zs[i];

    // Clamp to calibration range
    if (raw_x < CAL_X0) raw_x = CAL_X0;
    if (raw_x > CAL_X1) raw_x = CAL_X1;
    if (raw_y < CAL_Y0) raw_y = CAL_Y0;
    if (raw_y > CAL_Y1) raw_y = CAL_Y1;

    // Map calibrated raw (range -> 0..4095)
    const uint16_t dx = (CAL_X1 > CAL_X0) ? (CAL_X1 - CAL_X0) : 1;
    const uint16_t dy = (CAL_Y1 > CAL_Y0) ? (CAL_Y1 - CAL_Y0) : 1;
    const uint16_t cal_x = (uint16_t)(((uint32_t)(raw_x - CAL_X0) * 4095u) / dx);
    const uint16_t cal_y = (uint16_t)(((uint32_t)(raw_y - CAL_Y0) * 4095u) / dy);

    // Map 12-bit (0..4095) to pixel coordinates
    uint16_t px = (uint16_t)(((uint32_t)cal_x * LTDC_ACTIVE_WIDTH) / 4096u);
    uint16_t py = (uint16_t)(((uint32_t)cal_y * LTDC_ACTIVE_HEIGHT) / 4096u);

    // axis invert to make left/top
    px = (LTDC_ACTIVE_WIDTH - 1) - px;
    py = (LTDC_ACTIVE_HEIGHT - 1) - py;

    // Clamp to screen bounds
    if (px >= LTDC_ACTIVE_WIDTH) px = LTDC_ACTIVE_WIDTH - 1;
    if (py >= LTDC_ACTIVE_HEIGHT) py = LTDC_ACTIVE_HEIGHT - 1;

    coord.x = px;
    coord.y = py;
    coord.z = raw_z;
    return coord;
}
