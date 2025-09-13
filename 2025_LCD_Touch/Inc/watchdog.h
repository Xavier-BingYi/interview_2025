/*
 * watchdog.h
 *
 *  Created on: Sep 14, 2025
 *      Author: Xavier
 */

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

// Key values for IWDG (Independent Watchdog)
#define IWDG_KEY_RELOAD   0xAAAAU  // Reload the counter (feed the watchdog)
#define IWDG_KEY_ENABLE   0xCCCCU  // Start the watchdog
#define IWDG_KEY_ACCESS   0x5555U  // Unlock access to PR & RLR registers

// Status register bits
#define IWDG_SR_PVU (1U << 0)  // Prescaler Value Update ongoing
#define IWDG_SR_RVU (1U << 1)  // Reload Value Update ongoing

// Prescaler options (divide LSI clock)
typedef enum {
    IWDG_PRESCALER_4   = 0x0,  // divide by 4
    IWDG_PRESCALER_8   = 0x1,  // divide by 8
    IWDG_PRESCALER_16  = 0x2,  // divide by 16
    IWDG_PRESCALER_32  = 0x3,  // divide by 32
    IWDG_PRESCALER_64  = 0x4,  // divide by 64
    IWDG_PRESCALER_128 = 0x5,  // divide by 128
    IWDG_PRESCALER_256 = 0x6,  // divide by 256
    // 0x7 is also divide by 256
} IWDG_Prescaler;

// Function prototypes for IWDG control
void iwdg_init(void);                          // Initialize watchdog
void iwdg_write_kr(uint32_t key);              // Write to key register
uint32_t iwdg_read_sr(void);                   // Read status register
void iwdg_wait_pvu_clear(void);                // Wait until PVU bit is cleared
void iwdg_wait_rvu_clear(void);                // Wait until RVU bit is cleared
void iwdg_write_pr(IWDG_Prescaler prescaler);  // Set prescaler
void iwdg_write_rlr(uint32_t reload_12bit);    // Set reload value
void iwdg_start(void);                         // Start watchdog
void iwdg_feed(void);                          // Reload (feed) watchdog


#endif /* WATCHDOG_H_ */
