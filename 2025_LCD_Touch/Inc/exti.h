/*
 * exti.h
 *
 *  Created on: Jun 24, 2025
 *      Author: Xavier
 */

#ifndef EXTI_H_
#define EXTI_H_

// Port codes for SYSCFG_EXTICR (select EXTI GPIO source)
#define SYSCFG_EXTICR_PORTA  0x0  // PA[x]
#define SYSCFG_EXTICR_PORTB  0x1  // PB[x]
#define SYSCFG_EXTICR_PORTC  0x2  // PC[x]
#define SYSCFG_EXTICR_PORTD  0x3  // PD[x]
#define SYSCFG_EXTICR_PORTE  0x4  // PE[x]
#define SYSCFG_EXTICR_PORTF  0x5  // PF[x]
#define SYSCFG_EXTICR_PORTG  0x6  // PG[x]
#define SYSCFG_EXTICR_PORTH  0x7  // PH[x]
#define SYSCFG_EXTICR_PORTI  0x8  // PI[x]

// NVIC register bases
#define NVIC_ISER_BASE       0xE000E100U  // Interrupt Set-Enable Registers (ISER0 base)
#define NVIC_IPR_BASE        0xE000E400U  // Interrupt Priority Registers (IPR base)

// EXTI line index (maps PxN to EXTIx, N = 0..15)
typedef enum {
    SYSCFG_EXTI0  =  0,  // Px0
    SYSCFG_EXTI1  =  1,  // Px1
    SYSCFG_EXTI2  =  2,  // Px2
    SYSCFG_EXTI3  =  3,  // Px3
    SYSCFG_EXTI4  =  4,  // Px4
    SYSCFG_EXTI5  =  5,  // Px5
    SYSCFG_EXTI6  =  6,  // Px6
    SYSCFG_EXTI7  =  7,  // Px7
    SYSCFG_EXTI8  =  8,  // Px8
    SYSCFG_EXTI9  =  9,  // Px9
    SYSCFG_EXTI10 = 10,  // Px10
    SYSCFG_EXTI11 = 11,  // Px11
    SYSCFG_EXTI12 = 12,  // Px12
    SYSCFG_EXTI13 = 13,  // Px13
    SYSCFG_EXTI14 = 14,  // Px14
    SYSCFG_EXTI15 = 15   // Px15
} SYSCFG_EXTI_LINE;

// Mask control for EXTI interrupt
typedef enum {
    EXTI_INTERRUPT_DISABLE = 0,
    EXTI_INTERRUPT_ENABLE  = 1
} EXTI_InterruptMask;

// IRQ numbers (subset)
typedef enum {
    WWDG_IRQn       =  0,  // Window Watchdog
    PVD_IRQn        =  1,  // PVD via EXTI
    TAMP_STAMP_IRQn =  2,  // Tamper/TimeStamp via EXTI
    RTC_WKUP_IRQn   =  3,  // RTC Wakeup via EXTI
    FLASH_IRQn      =  4,  // Flash global
    RCC_IRQn        =  5,  // RCC global
    EXTI0_IRQn      =  6,  // EXTI line 0
    EXTI15_10_IRQn  = 40,  // EXTI lines 15..10
    TIM7_IRQn       = 55   // TIM7 global
} IRQn_Type;

// NVIC priority levels (lower value = higher priority)
typedef enum {
    NVIC_PRIORITY_0  = 0x00,
    NVIC_PRIORITY_1  = 0x10,
    NVIC_PRIORITY_2  = 0x20,
    NVIC_PRIORITY_3  = 0x30,
    NVIC_PRIORITY_4  = 0x40,
    NVIC_PRIORITY_5  = 0x50,
    NVIC_PRIORITY_6  = 0x60,
    NVIC_PRIORITY_7  = 0x70,
    NVIC_PRIORITY_8  = 0x80,
    NVIC_PRIORITY_9  = 0x90,
    NVIC_PRIORITY_10 = 0xA0,
    NVIC_PRIORITY_11 = 0xB0,
    NVIC_PRIORITY_12 = 0xC0,
    NVIC_PRIORITY_13 = 0xD0,
    NVIC_PRIORITY_14 = 0xE0,
    NVIC_PRIORITY_15 = 0xF0
} NVIC_Priority;

extern volatile uint8_t button_event_pending; // button event flag (set in EXTI0 IRQ)
extern volatile uint8_t touch_event_pending;  // touch event flag (set in EXTI15_10 IRQ)

void exti_init(void);                                                     // init EXTI/NVIC
void exti_select_port(SYSCFG_EXTI_LINE exti_line, uint8_t port_code);    // map EXTI line to GPIO port
void exti_enable_rising_trigger(SYSCFG_EXTI_LINE exti_line);             // enable rising edge trigger
void exti_enable_falling_trigger(SYSCFG_EXTI_LINE exti_line);            // enable falling edge trigger
void exti_set_interrupt_mask(SYSCFG_EXTI_LINE exti_line, EXTI_InterruptMask enable); // mask/unmask interrupt
void exti_clear_pending_flag(SYSCFG_EXTI_LINE exti_line);                 // clear EXTI pending bit

void nvic_enable_irq(IRQn_Type irqn);                                     // enable NVIC IRQ
void nvic_set_priority(IRQn_Type irqn, NVIC_Priority priority);           // set NVIC priority


#endif /* EXTI_H_ */
