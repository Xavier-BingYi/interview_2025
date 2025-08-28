/*
 * i2c.h
 *
 *  Created on: Aug 21, 2025
 *      Author: Xavier
 */

#ifndef I2C_H_
#define I2C_H_

typedef enum {
    I2C_CR1_PE        = 0,   // Peripheral enable
    I2C_CR1_SMBUS     = 1,   // SMBus mode
    I2C_CR1_RSVD2     = 2,   // (Reserved, do not use)
    I2C_CR1_SMBTYPE   = 3,   // SMBus type (0: device, 1: host)
    I2C_CR1_ENARP     = 4,   // ARP enable
    I2C_CR1_ENPEC     = 5,   // PEC enable
    I2C_CR1_ENGC      = 6,   // General call enable
    I2C_CR1_NOSTRETCH = 7,   // Clock stretching disable
    I2C_CR1_START     = 8,   // START generation
    I2C_CR1_STOP      = 9,   // STOP generation
    I2C_CR1_ACK       = 10,  // Acknowledge enable
    I2C_CR1_POS       = 11,  // POS: ACK/PEC position (for reception)
    I2C_CR1_PEC       = 12,  // Packet error checking (transfer PEC)
    I2C_CR1_ALERT     = 13,  // SMBus alert
    I2C_CR1_RSVD14    = 14,  // (Reserved, do not use)
    I2C_CR1_SWRST     = 15   // Software reset
} i2c_cr1_field_t;

typedef enum {
    I2C_CR2_FREQ      = 0,   // FREQ[5:0] = APB1 clock in MHz (multi-bit field, width=6)
    I2C_CR2_RSVD6     = 6,   // Reserved
    I2C_CR2_RSVD7     = 7,   // Reserved
    I2C_CR2_ITERREN   = 8,   // Error interrupt enable
    I2C_CR2_ITEVTEN   = 9,   // Event interrupt enable
    I2C_CR2_ITBUFEN   = 10,  // Buffer interrupt enable
    I2C_CR2_DMAEN     = 11,  // DMA requests enable
    I2C_CR2_LAST      = 12,  // DMA last transfer
    I2C_CR2_RSVD13    = 13,  // Reserved
    I2C_CR2_RSVD14    = 14,  // Reserved
    I2C_CR2_RSVD15    = 15   // Reserved
} i2c_cr2_field_t;

typedef enum {
    I2C_CCR_CCR      = 0,   // CCR[11:0]，12-bit divider
    I2C_CCR_RSVD12   = 12,  // Reserved
    I2C_CCR_RSVD13   = 13,  // Reserved
    I2C_CCR_DUTY     = 14,  // Fast-mode only：0=>Tlow/Thigh=2，1=>16/9
    I2C_CCR_FS       = 15   // 0=Standard-mode(≤100kHz)；1=Fast-mode(≤400kHz)
} i2c_ccr_field_t;

typedef enum {
    I2C_SR1_SB        = 0,   // Start bit (Master mode)
    I2C_SR1_ADDR      = 1,   // Address sent/matched
    I2C_SR1_BTF       = 2,   // Byte transfer finished
    I2C_SR1_ADD10     = 3,   // 10-bit header sent
    I2C_SR1_STOPF     = 4,   // Stop detection
    // bit5 Reserved
    I2C_SR1_RXNE      = 6,   // Data register not empty (receive)
    I2C_SR1_TXE       = 7,   // Data register empty (transmit)
    I2C_SR1_BERR      = 8,   // Bus error
    I2C_SR1_ARLO      = 9,   // Arbitration lost
    I2C_SR1_AF        = 10,  // Acknowledge failure
    I2C_SR1_OVR       = 11,  // Overrun/Underrun
    I2C_SR1_PECERR    = 12,  // PEC error
    // bit13 Reserved
    I2C_SR1_TIMEOUT   = 14,  // Timeout/Tlow error
    I2C_SR1_SMBALERT  = 15   // SMBus alert
} i2c_sr1_field_t;

typedef enum {
    I2C_SR2_MSL       = 0,   // Master/Slave
    I2C_SR2_BUSY      = 1,   // Bus busy
    I2C_SR2_TRA       = 2,   // Transmitter/Receiver
    // bit3 Reserved
    I2C_SR2_GENCALL   = 4,   // General call
    I2C_SR2_SMBDEFAULT= 5,   // SMBus device default address
    I2C_SR2_SMBHOST   = 6,   // SMBus host
    I2C_SR2_DUALF     = 7,   // Dual flag
    I2C_SR2_PEC       = 8,   // PEC
} i2c_sr2_field_t;

#define SLAVE_ADDR7     0x41u

#define SYS_CTRL2_ADDR  0x04u
#define INT_CTRL_ADDR   0x09u
#define INT_EN_ADDR     0x0Au
#define INT_STA_ADDR    0x0Bu
#define TSC_CTRL_ADDR   0x40u
#define TSC_CFG_ADDR    0x41u
#define TSC_DATA_XYZ_ADDR 0x52

// SYS_CTRL2 bits: [3]=TS_OFF, [2]=GPIO_OFF, [1]=TSC_OFF, [0]=ADC_OFF (1 = clock OFF)
#define SYS_TS_OFF    (1u << 3)
#define SYS_GPIO_OFF  (1u << 2)
#define SYS_TSC_OFF   (1u << 1)
#define SYS_ADC_OFF   (1u << 0)

// INT_CTRL bits: [2]=INT_POLARITY, [1]=INT_TYPE, [0]=GLOBAL_INT
#define INT_POLARITY  (1u << 2)   // 1 = active high / rising, 0 = active low / falling
#define INT_TYPE      (1u << 1)   // 1 = edge, 0 = level
#define GLOBAL_INT    (1u << 0)   // 1 = enable global interrupt

// INT_EN (0x0A): enable sources
#define INT_EN_TOUCH_DET (1u << 0)  // Touch detected

// TSC_CTRL bits: [6:4]=TRACK, [3:1]=OP_MOD, [0]=EN
#define TSC_CTRL_TRACK_SHIFT 4
#define TSC_CTRL_OPMOD_SHIFT 1
#define TSC_CTRL_EN          (1u << 0)

// TSC_CFG bits: [7:6]=AVE_CTRL, [5:3]=TOUCH_DET_DELAY, [2:0]=SETTLING
#define TSC_CFG_AVE_SHIFT   6   // 00=1, 01=2, 10=4, 11=8 samples
#define TSC_CFG_TDLY_SHIFT  3   // 000=10us ... 111=50ms
// SETTLING (bits [2:0]): 000=10us,001=100us,010=500us,011=1ms,100=5ms,101=10ms,110=50ms,111=100ms

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  z;
} TouchPoint;

void i2c_init(void);
void i2c_cr1_write_field(uint32_t i2c_base, i2c_cr1_field_t field, uint32_t value);
void i2c_cr2_write_field(uint32_t i2c_base, i2c_cr2_field_t field, uint32_t value);
void i2c_ccr_write_field(uint32_t i2c_base, i2c_ccr_field_t field, uint32_t value);
void i2c_trise_write_field(uint32_t i2c_base, uint32_t value);
void i2c_sr1_write_field(uint32_t i2c_base, i2c_sr1_field_t field, uint32_t value);
uint8_t i2c_sr1_read_field(uint32_t i2c_base, i2c_sr1_field_t field);
uint8_t i2c_sr2_read_field(uint32_t i2c_base, i2c_sr2_field_t field);
int8_t i2c_master_write(uint32_t i2c_base, uint8_t slave_addr7, uint8_t reg, uint8_t data);
int8_t i2c_master_read(uint32_t i2c_base, uint8_t slave_addr7, uint8_t reg_addr, uint8_t *data, uint16_t len);
void i2c_touch_init(void);
TouchPoint i2c_touch_read_xyz(void);



#endif /* I2C_H_ */
