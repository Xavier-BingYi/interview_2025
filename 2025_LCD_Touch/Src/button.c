/*
 * button.c
 *
 *  Created on: Aug 27, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <button.h>
#include <timer.h>
#include <gpio.h>

// --- LED states ---
uint8_t red_led_state = 0;              // red LED (0=off, 1=on)
volatile uint8_t green_led_state = 0;            // green LED (0=off, 1=on)

static uint32_t last_press_us = 0;   // last accepted button time (us)

// --- Green LED periodic toggle ---
void green_led_blink_update(void) {
    /*
     * static uint32_t green_led_now_us = 0;    // current time (us) for LED
     * static int32_t  green_led_diff   = 0;    // time since deadline
     *
	 * uint32_t green_led_deadline_us = 0;          // next time to toggle green LED (us)
     * green_led_now_us = micros_now(TIMER2);
    green_led_diff   = (int32_t)(green_led_now_us - green_led_next_deadline);

    if (green_led_diff >= 0) {                              // overdue -> toggle
        uint32_t missed = (uint32_t)green_led_diff / GREEN_LED_PERIOD_US + 1U;
        green_led_next_deadline += missed * GREEN_LED_PERIOD_US;

        green_led_on ^= 1;
        gpio_set_outdata(GPIOG_BASE, GPIO_PIN_13, green_led_on);
    }*/
}

// --- Button event handler (debounced) ---
void button_handle_event(void) {
    uint32_t now = micros_now(TIMER2);            // read time (us)
    if ((uint32_t)(now - last_press_us) < BUTTON_DEBOUNCE_US) return; // inside debounce

    last_press_us = now;                           // accept press

    red_led_state ^= 1;                               // toggle red LED
    gpio_set_outdata(GPIOG_BASE, GPIO_PIN_14, red_led_state);
}
