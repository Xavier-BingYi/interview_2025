/*
 * lcd_render.c
 *
 *  Created on: Aug 26, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <gpio.h>
#include <ltdc.h>
#include <usart.h>
#include <exti.h>
#include <timer.h>
#include <lcd_render.h>
#include <i2c.h>

static uint32_t last_touch_us = 0;            // last accepted touch timestamp (us)
static uint32_t last_touch_state = 0;
volatile uint8_t touch_state = 0;             // 0=rotation mode, 1=touch mode
volatile uint32_t touch_until_us = 0;         // touch mode deadline (us)
volatile uint8_t lcd_rotation_state = 0;      // current rotation frame
TouchPoint touch_coord = {0};

void touch_handle_event(void) {
    uint32_t now = micros_now(TIMER2);        // current time (us)
    if ((uint32_t)(now - last_touch_us) < TOUCH_DEBOUNCE_US) return; // ignore within debounce window

    last_touch_state = lcd_rotation_state;
    touch_lock_for_ms(2000);                  // enter touch mode for 2 s
    last_touch_us = now;                      // update last touch time
}

void touch_lock_for_ms(uint32_t ms) {
    uint32_t now = micros_now(TIMER2);        // current time (us)
    touch_state    = 1;                       // enable touch mode

    touch_coord = i2c_touch_read_xyz();     // read touch coordinates
    usart_printf("%d  %d  %d  \r\n", touch_coord.x, touch_coord.y, touch_coord.z); // log coordinates

    touch_until_us = now + ms * 1000u;        // time of touch mode end
}

static inline void draw_quadrants(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br) {
    // bottom-left
    bsp_lcd_fill_rect(tl, X0, X1, Y0, Y1);
    // top-left
    bsp_lcd_fill_rect(tr, X1, X2, Y0, Y1);
    // bottom-right
    bsp_lcd_fill_rect(bl, X0, X1, Y1, Y2);
    // top-right
    bsp_lcd_fill_rect(br, X1, X2, Y1, Y2);
}

void lcd_update(void) {
    if (touch_state) {                         // touch mode screen
        if (last_touch_state == 0) {
            // TL=NAVY, TR=ORANGE, BL=LGHTGRAY, BR=TEAL
            if (touch_coord.x < X1) {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_NAVY);
                else                     fill_framebuffer_rgb888(COL_LGHTGRAY);
            } else {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_ORANGE);
                else                     fill_framebuffer_rgb888(COL_TEAL);
            }

        } else if (last_touch_state == 1) {
            // TL=LGHTGRAY, TR=NAVY, BL=TEAL, BR=ORANGE
            if (touch_coord.x < X1) {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_LGHTGRAY);
                else                     fill_framebuffer_rgb888(COL_TEAL);
            } else {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_NAVY);
                else                     fill_framebuffer_rgb888(COL_ORANGE);
            }

        } else if (last_touch_state == 2) {
            // TL=TEAL, TR=LGHTGRAY, BL=ORANGE, BR=NAVY
            if (touch_coord.x < X1) {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_TEAL);
                else                     fill_framebuffer_rgb888(COL_ORANGE);
            } else {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_LGHTGRAY);
                else                     fill_framebuffer_rgb888(COL_NAVY);
            }

        } else if (last_touch_state == 3) {
            // TL=ORANGE, TR=TEAL, BL=NAVY, BR=LGHTGRAY
            if (touch_coord.x < X1) {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_ORANGE);
                else                     fill_framebuffer_rgb888(COL_NAVY);
            } else {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_TEAL);
                else                     fill_framebuffer_rgb888(COL_LGHTGRAY);
            }

        } else {
            // fallback：同 state 0
            if (touch_coord.x < X1) {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_NAVY);
                else                     fill_framebuffer_rgb888(COL_LGHTGRAY);
            } else {
                if (touch_coord.y < Y1) fill_framebuffer_rgb888(COL_ORANGE);
                else                     fill_framebuffer_rgb888(COL_TEAL);
            }
        }

    } else {                                    // rotation mode screen
        switch (lcd_rotation_state) {
        case 0:
            draw_quadrants(COL_NAVY,    COL_ORANGE,  COL_LGHTGRAY, COL_TEAL);
            break;
        case 1:
        	draw_quadrants(COL_LGHTGRAY, COL_NAVY,   COL_TEAL,     COL_ORANGE);
            break;
        case 2:
        	draw_quadrants(COL_TEAL,    COL_LGHTGRAY, COL_ORANGE,  COL_NAVY);
            break;
        case 3:
        	draw_quadrants(COL_ORANGE,  COL_TEAL,    COL_NAVY,     COL_LGHTGRAY);
            break;
        default:
            // draw color bars
            bsp_lcd_fill_rect(0xEE82EE, 0, 240, 46*0, 46); // Violet
            bsp_lcd_fill_rect(0x4B0082, 0, 240, 46*1, 46); // Indigo
            bsp_lcd_fill_rect(0x0000FF, 0, 240, 46*2, 46); // Blue
            bsp_lcd_fill_rect(0x008000, 0, 240, 46*3, 46); // Green
            bsp_lcd_fill_rect(0xFFFF00, 0, 240, 46*4, 46); // Yellow
            bsp_lcd_fill_rect(0xFFA500, 0, 240, 46*5, 46); // Orange
            bsp_lcd_fill_rect(0xFF0000, 0, 240, 46*6, 44); // Red
            break;
        }
    }
}
