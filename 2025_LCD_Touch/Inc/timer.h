/*
 * timer.h
 *
 *  Created on: Aug 17, 2025
 *      Author: Xavier
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TIMER1 = 1,
    TIMER2 = 2,
    TIMER3 = 3,
    TIMER4 = 4,
    TIMER5 = 5,
    TIMER6 = 6,
    TIMER7 = 7
} timer_id_t;

typedef enum {
	TIMx_CR1_CEN  = 0,
	TIMx_CR1_UDIS = 1,
	TIMx_CR1_URS  = 2,
	TIMx_CR1_OPM  = 3,
	TIMx_CR1_DIR  = 4,
	TIMx_CR1_CMS  = 5,
	TIMx_CR1_ARPE = 7,
	TIMx_CR1_CKD  = 8
} timer_cr1_field_t;

typedef enum {
    //TIMX_DIER_UIE = 0U,
    //TIMX_DIER_UDE = 0x100U,
	TIMX_DIER_UIE = (1U << 0),
	TIMX_DIER_UDE = (1U << 8),
} timer_dir_field_t;

typedef enum {
    TIMx_EGR_UG   = 0,  // Update event
    TIMx_EGR_CC1G = 1,  // CC1 event (not in TIM6/7)
    TIMx_EGR_CC2G = 2,  // CC2 event (not in TIM6/7)
    TIMx_EGR_CC3G = 3,  // CC3 event (not in TIM6/7)
    TIMx_EGR_CC4G = 4,  // CC4 event (not in TIM6/7)
    // bit5: reserved
    TIMx_EGR_TG   = 6   // Trigger event (not in TIM6/7)
} timer_egr_field_t;



void timer_init(void);
void timer7_start_oneshot_ms(uint16_t ms);
void timer_cr1_set_field(timer_id_t id, timer_cr1_field_t field, uint32_t value);
void timer_set_dier(timer_id_t id, timer_dir_field_t bit);
void timer_set_sr(timer_id_t id, uint32_t bit);
void timer_egr_set_field(timer_id_t id, timer_egr_field_t field, uint32_t value);
void timer_set_psc(timer_id_t id, uint32_t psc_value);
void timer_set_arr(timer_id_t id, uint32_t arr_value);
uint32_t micros_now(timer_id_t id);



#endif /* TIMER_H_ */
