/*
 * button.h
 *
 *  Created on: Aug 27, 2025
 *      Author: Xavier
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#define GREEN_LED_PERIOD_US   1000000U   // LED blink period (1 s)
#define BUTTON_DEBOUNCE_US      10000U   // button debounce interval (10 ms)

extern uint8_t red_led_state;            // current state of red LED
extern volatile uint8_t green_led_state; // current state of green LED
extern uint32_t green_led_deadline_us;   // next deadline for green LED toggle (us)

void green_led_blink_update(void);       // update green LED state
void button_handle_event(void);          // handle button press

#endif /* BUTTON_H_ */
