/*
 * exti.h
 *
 *  Created on: Jun 24, 2025
 *      Author: Xavier
 */

#ifndef EXTI_H_
#define EXTI_H_

/* EXTI line numbers for mapping PxN to EXTIx (N = 0~15) */
typedef enum {
    SYSCFG_EXTI0  = 0,   // Px0
    SYSCFG_EXTI1  = 1,   // Px1
    SYSCFG_EXTI2  = 2,   // Px2
    SYSCFG_EXTI3  = 3,   // Px3
    SYSCFG_EXTI4  = 4,   // Px4
    SYSCFG_EXTI5  = 5,   // Px5
    SYSCFG_EXTI6  = 6,   // Px6
    SYSCFG_EXTI7  = 7,   // Px7
    SYSCFG_EXTI8  = 8,   // Px8
    SYSCFG_EXTI9  = 9,   // Px9
    SYSCFG_EXTI10 = 10,  // Px10
    SYSCFG_EXTI11 = 11,  // Px11
    SYSCFG_EXTI12 = 12,  // Px12
    SYSCFG_EXTI13 = 13,  // Px13
    SYSCFG_EXTI14 = 14,  // Px14
    SYSCFG_EXTI15 = 15   // Px15
} SYSCFG_EXTI_LINE;

/* Port code definitions for SYSCFG_EXTICR (used to select EXTI GPIO source) */
#define SYSCFG_EXTICR_PORTA  0x0  // PA[x] pin
#define SYSCFG_EXTICR_PORTB  0x1  // PB[x] pin
#define SYSCFG_EXTICR_PORTC  0x2  // PC[x] pin
#define SYSCFG_EXTICR_PORTD  0x3  // PD[x] pin
#define SYSCFG_EXTICR_PORTE  0x4  // PE[x] pin
#define SYSCFG_EXTICR_PORTF  0x5  // PF[x] pin
#define SYSCFG_EXTICR_PORTG  0x6  // PG[x] pin
#define SYSCFG_EXTICR_PORTH  0x7  // PH[x] pin
#define SYSCFG_EXTICR_PORTI  0x8  // PI[x] pin

typedef enum{
    EXTI_INTERRUPT_DISABLE = 0,
    EXTI_INTERRUPT_ENABLE  = 1
}EXTI_InterruptMask;

#define NVIC_ISER_BASE  0xE000E100U  // NVIC Interrupt Set-Enable Registers (ISER0 base)

typedef enum {
    WWDG        = 0,   // Window Watchdog interrupt
    PVD         = 1,   // PVD through EXTI line detection
    TAMP_STAMP  = 2,   // Tamper and TimeStamp interrupts
    RTC_WKUP    = 3,   // RTC Wakeup interrupt
    FLASH       = 4,   // Flash global interrupt
    RCC         = 5,   // RCC global interrupt
    EXTI0       = 6    // EXTI Line0 interrupt
} IRQn;

void exti_init(void);
void exti_select_port(SYSCFG_EXTI_LINE exti_line, uint8_t port_code);
void exti_enable_rising_trigger(SYSCFG_EXTI_LINE exti_line);
void exti_enable_falling_trigger(SYSCFG_EXTI_LINE exti_line);
void exti_set_interrupt_mask(SYSCFG_EXTI_LINE exti_line, EXTI_InterruptMask enable);
void nvic_enable_irq(IRQn irqn);


#endif /* EXTI_H_ */
