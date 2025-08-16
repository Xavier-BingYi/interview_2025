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

void exti_gpio_init(void){
	rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
	gpio_set_mode(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_INPUT);
}

void exti_init(void) {
	exti_gpio_init();

	rcc_enable_apb2_clock(RCC_APB2EN_SYSCFG);
	exti_select_port(SYSCFG_EXTI0, SYSCFG_EXTICR_PORTA);
	exti_enable_rising_trigger(SYSCFG_EXTI0);
	exti_set_interrupt_mask(SYSCFG_EXTI0, EXTI_INTERRUPT_ENABLE);
	nvic_enable_irq(EXTI0);
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

void nvic_enable_irq(IRQn irqn) {
    uint32_t reg_addr = NVIC_ISER_BASE + ((uint32_t)irqn / 32) * 4;  // @param  irqn: IRQ number (Interrupt Request Number)
    uint32_t bit_mask = 1U << ((uint32_t)irqn % 32);
    io_writeMask(reg_addr, bit_mask, bit_mask);
}

volatile uint8_t lcd_button_state = 0;

void EXTI0_IRQHandler(void) {
    exti_clear_pending_flag(SYSCFG_EXTI0);

    delay_us(100000);

    if (lcd_button_state == 0){
    	lcd_button_state++;}


	if (gpio_read_idr(GPIOG_BASE, GPIO_PIN_14) == 1)
		gpio_set_outdata(GPIOG_BASE, GPIO_PIN_14, 0);
	else
		gpio_set_outdata(GPIOG_BASE, GPIO_PIN_14, 1);

    // usart_printf("Button Pressed!\r\n");
}
