/*
 * watchdog.c
 *
 *  Created on: Sep 14, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <mem_io.h>
#include <mem_map.h>
#include <gpio.h>
#include <watchdog.h>

void iwdg_init(void) {
    // Set prescaler (clock divider) for IWDG counter
    iwdg_write_pr(IWDG_PRESCALER_128);

    // Set reload value (maximum: 0x0FFF)
    iwdg_write_rlr(0x0FFFU);

    // Start the independent watchdog
    iwdg_start();

    // Feed the watchdog once after starting
    iwdg_feed();
}

void iwdg_write_kr(uint32_t key) {
	io_write(IWDG_BASE + IWDG_KR_OFFSET, key);
}

uint32_t iwdg_read_sr(void) {
	return io_read(IWDG_BASE + IWDG_SR_OFFSET);
}

void iwdg_wait_pvu_clear(void) {
    while (iwdg_read_sr() & IWDG_SR_PVU) {}
}

void iwdg_wait_rvu_clear(void) {
    while (iwdg_read_sr() & IWDG_SR_RVU) {}
}

void iwdg_write_pr(IWDG_Prescaler prescaler) {
    iwdg_write_kr(IWDG_KEY_ACCESS);
    iwdg_wait_pvu_clear();
    io_write(IWDG_BASE + IWDG_PR_OFFSET, prescaler & 0x07U);     // PR[2:0]
}

void iwdg_write_rlr(uint32_t reload_12bit) {
    iwdg_write_kr(IWDG_KEY_ACCESS);
    iwdg_wait_rvu_clear();
    io_write(IWDG_BASE + IWDG_RLR_OFFSET, reload_12bit & 0x0FFFU); // RL[11:0]
}

void iwdg_start(void) {
	iwdg_write_kr(IWDG_KEY_ENABLE);
}

void iwdg_feed(void) {
	iwdg_write_kr(IWDG_KEY_RELOAD);
}
