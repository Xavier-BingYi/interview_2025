/*
 * timer.c
 *
 *  Created on: Aug 17, 2025
 *      Author: Xavier
 */


#include <stdint.h>
#include <stdbool.h>
#include <rcc.h>
#include <timer.h>
#include <mem_io.h>
#include <mem_map.h>
#include <exti.h>

void timer_init(void) {
	rcc_enable_apb1_clock(RCC_APB1EN_TIM2EN);
	rcc_enable_apb1_clock(RCC_APB1EN_TIM7EN);

    // --- TIM2: free-running timestamp ---
    timer_set_psc(TIMER2, 89);            // CK_CNT = 1 MHz
    timer_set_arr(TIMER2, 0xFFFFFFFFU);   // 32-bit free-running
    timer_cr1_set_field(TIMER2, TIMx_CR1_URS, 1); // avoid set UIF
    timer_egr_set_field(TIMER2, TIMx_EGR_UG, 1);  // update the PSC & ARR
    timer_cr1_set_field(TIMER2, TIMx_CR1_URS, 0);
    timer_cr1_set_field(TIMER2, TIMx_CR1_CEN, 1); // start

    // --- TIM7: used for debounce one-shot (1 kHz base) ---
    timer_set_psc(TIMER7, 8999);        // CK_CNT = 90 MHz / (8999+1) = 10 kHz
    timer_set_arr(TIMER7, 9999);        // count (9999 + 1) times

    timer_cr1_set_field(TIMER7, TIMx_CR1_URS, 1);
    timer_egr_set_field(TIMER7, TIMx_EGR_UG, 1);
    timer_cr1_set_field(TIMER7, TIMx_CR1_URS, 0);

    timer_set_dier(TIMER7, TIMX_DIER_UIE);

    timer_cr1_set_field(TIMER7, TIMx_CR1_CEN, 1);

    nvic_enable_irq(TIM7_IRQn);     // 只先開 NVIC，UIE 由啟動時再打開
}

void timer_cr1_set_field(timer_id_t id, timer_cr1_field_t field, uint32_t value) {
	uint32_t addr;
	uint32_t width = 0;
    uint32_t shift = field;

	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_CR1_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_CR1_OFFSET;
	} else return;

    switch (field) {
        case TIMx_CR1_CEN:  width = 1; break;
        case TIMx_CR1_UDIS: width = 1; break;
        case TIMx_CR1_URS:  width = 1; break;
        case TIMx_CR1_OPM:  width = 1; break;
        case TIMx_CR1_DIR:  width = 1; break;
        case TIMx_CR1_CMS:  width = 2; break;
        case TIMx_CR1_ARPE: width = 1; break;
        case TIMx_CR1_CKD:  width = 2; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = (value & ((1U << width) - 1U)) << shift;

    io_writeMask(addr, data, mask);
}

void timer_set_dier(timer_id_t id, timer_dir_field_t bit) {
	uint32_t addr;
	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_DIER_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_DIER_OFFSET;
	} else return;

	io_writeMask(addr, bit, bit);
}

void timer_set_sr(timer_id_t id, uint32_t bit) {
	uint32_t addr;
	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_SR_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_SR_OFFSET;
	} else return;

	io_write(addr, bit);
}

void timer_egr_set_field(timer_id_t id, timer_egr_field_t field, uint32_t value) {
	uint32_t addr;
    uint32_t shift = field;

	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_EGR_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_EGR_OFFSET;
	} else return;

    uint32_t mask = 1U << shift;
    uint32_t data = value << shift;

    io_writeMask(addr, data, mask);
}

void timer_set_psc(timer_id_t id, uint32_t psc_value) {
	uint32_t addr;
	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_PSC_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_PSC_OFFSET;
	} else return;

	io_write(addr, psc_value);
}

void timer_set_arr(timer_id_t id, uint32_t arr_value) {
	uint32_t addr;
	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_ARR_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_ARR_OFFSET;
	} else return;

	if (id == TIMER6 || id == TIMER7) arr_value &= 0xFFFFU;
	io_write(addr, arr_value);
}

uint32_t micros_now(timer_id_t id) {
	uint32_t addr;
	if (id >= 2 && id <= 7){
		addr = TIM_2_TO_7_BASE(id) + TIMx_CNT_OFFSET;
	} else if (id >= 12 && id <= 14){
		addr = TIM_12_TO_14_BASE(id) + TIMx_CNT_OFFSET;
	} else return 0;

    return io_read(addr);
}
