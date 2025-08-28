/*
 * touch.h
 *
 *  Created on: Aug 26, 2025
 *      Author: Xavier
 */

#ifndef TOUCH_H_
#define TOUCH_H_

#include <lcd_render.h>

#define TOUCH_DEBOUNCE_US 300000U            // debounce time (300 ms)

#define X0 0
#define X1 120
#define X2 240
#define Y0 0
#define Y1 160
#define Y2 320

#define COL_RED      0xFF0000   // red
#define COL_INDIGO   0x4B0082   // indigo
#define COL_WHITE    0xFFFFFF   // white
#define COL_BLUE     0x0000FF   // blue
#define COL_PINK     0xFF99CC   // pink
#define COL_LTBLUE   0x66CCFF   // light blue
#define COL_MINT     0x99FFCC   // mint
#define COL_LAVENDER 0xCC99FF   // lavender
#define COL_NAVY     0x003366   // navy blue
#define COL_ORANGE   0xFF6600   // orange
#define COL_LGHTGRAY 0xDDDDDD   // light gray
#define COL_TEAL     0x009999   // teal

extern volatile uint8_t lcd_rotation_state; // current rotation frame
extern volatile uint8_t touch_state;        // 0=rotation mode, 1=touch mode
extern volatile uint32_t touch_until_us;    // touch mode deadline (us)

void lcd_update(void);                    // update LCD display
void touch_handle_event(void);            // process touch event
void touch_lock_for_ms(uint32_t ms);      // enter touch mode for given ms

#endif /* TOUCH_H_ */
