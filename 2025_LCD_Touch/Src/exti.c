/*
 * exti.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Xavier
 */

#include <stdint.h>
#include <rcc.h>
#include <gpio.h>
#include <usart.h>
#include <mem_io.h>
#include <mem_map.h>
#include <exti.h>
#include <ltdc.h>
#include <timer.h>
#include <i2c.h>
#include <lcd_render.h>
#include <button.h>

volatile uint8_t button_event_pending = 0;
volatile uint8_t touch_event_pending = 0;

void exti_gpio_init(void) {
	rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
	gpio_set_mode(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_INPUT);
    gpio_set_pupdr(GPIOA_BASE, GPIO_PIN_0, GPIO_PUPD_NONE);


	gpio_set_mode(GPIOA_BASE, GPIO_PIN_15, GPIO_MODE_INPUT);
    gpio_set_pupdr(GPIOA_BASE, GPIO_PIN_15, GPIO_PUPD_NONE);
}

void exti_init(void) {
	exti_gpio_init();

    // SYSCFG clock for EXTI mux
    rcc_enable_apb2_clock(RCC_APB2EN_SYSCFG);

	exti_select_port(SYSCFG_EXTI0, SYSCFG_EXTICR_PORTA);
	exti_enable_rising_trigger(SYSCFG_EXTI0);
	exti_set_interrupt_mask(SYSCFG_EXTI0, EXTI_INTERRUPT_ENABLE);
    exti_clear_pending_flag(SYSCFG_EXTI0); // clear EXTI line0 pending

	nvic_enable_irq(EXTI0_IRQn);



    // Map EXTI line15 to port A, enable falling trigger
    exti_select_port(SYSCFG_EXTI15, SYSCFG_EXTICR_PORTA);
    exti_enable_falling_trigger(SYSCFG_EXTI15);
    exti_set_interrupt_mask(SYSCFG_EXTI15, EXTI_INTERRUPT_ENABLE);
    exti_clear_pending_flag(SYSCFG_EXTI15); // clear EXTI line15 pending

    // NVIC for EXTI lines 10..15
    nvic_enable_irq(EXTI15_10_IRQn);
}

void exti_select_port(SYSCFG_EXTI_LINE exti_line, uint8_t port_code){
    uint32_t reg_addr;
    uint32_t shift = (exti_line % 4) * 4;
    uint32_t mask = 15U << shift;
    uint32_t data = (uint32_t)port_code << shift;

    switch (exti_line / 4){
    	case 0:
    		reg_addr = SYSCFG_BASE + SYSCFG_EXTICR1_OFFSET;
    		break;
    	case 1:
    		reg_addr = SYSCFG_BASE + SYSCFG_EXTICR2_OFFSET;
    		break;
    	case 2:
    		reg_addr = SYSCFG_BASE + SYSCFG_EXTICR3_OFFSET;
    		break;
    	case 3:
    		reg_addr = SYSCFG_BASE + SYSCFG_EXTICR4_OFFSET;
    		break;
    	default:
    		return;
    }

    io_writeMask(reg_addr, data, mask);
}

void exti_enable_rising_trigger(SYSCFG_EXTI_LINE exti_line){
    uint32_t reg_addr = EXTI_BASE + EXTI_RTSR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);
}

void exti_enable_falling_trigger(SYSCFG_EXTI_LINE exti_line){
    uint32_t reg_addr = EXTI_BASE + EXTI_FTSR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);
}

void exti_set_interrupt_mask(SYSCFG_EXTI_LINE exti_line, EXTI_InterruptMask enable){
    uint32_t reg_addr = EXTI_BASE + EXTI_IMR_OFFSET;
    uint32_t mask = 1U << exti_line;
    uint32_t data = (uint32_t)enable << exti_line;

    io_writeMask(reg_addr, mask, data);
}

void exti_clear_pending_flag(SYSCFG_EXTI_LINE exti_line) {
    uint32_t reg_addr = EXTI_BASE + EXTI_PR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);
}

void nvic_enable_irq(IRQn_Type irqn) {
	if ((int32_t)irqn < 0) return;

	uint32_t irqn_u = (uint32_t)irqn;
	uint32_t reg_idx  = irqn_u / 32;
    uint32_t reg_addr = NVIC_ISER_BASE + (reg_idx * 4);  // @param  irqn: IRQ number (Interrupt Request Number)

    uint32_t mask = 1U << (irqn_u % 32);

    io_writeMask(reg_addr, mask, mask);
}

void nvic_set_priority(IRQn_Type irqn, NVIC_Priority priority) {
	if ((int32_t)irqn < 0) return;

	uint32_t irqn_u = (uint32_t)irqn;
	uint32_t ipr_idx  = (irqn_u / 4);
	uint32_t reg_addr = NVIC_IPR_BASE + ipr_idx * 4;
	uint32_t shift    = (irqn_u % 4) * 8;

	uint32_t mask = 0xFFu << shift;

    io_writeMask(reg_addr, (uint32_t)priority << shift, mask);
}

void EXTI0_IRQHandler(void) {
    exti_clear_pending_flag(SYSCFG_EXTI0);    // clear EXTI line0 pending
    button_event_pending = 1;                 // set button event flag
}

void TIM7_IRQHandler(void) {
    timer_set_sr(TIMER7, 0);                  // clear TIM7 interrupt flag

    //lcd_time_state = (uint8_t)((lcd_time_state + 1) & 3);

    if (++lcd_rotation_state > 3)             // update rotation state (0–3)
        lcd_rotation_state = 0;

    green_led_state ^= 1;                     // toggle LED state
    gpio_set_outdata(GPIOG_BASE, GPIO_PIN_13, green_led_state); // update LED output
}

void EXTI15_10_IRQHandler(void) {
    exti_clear_pending_flag(SYSCFG_EXTI15);   // clear EXTI line15 pending
    touch_event_pending = 1;                  // set touch event flag
}
