/*
 * hooks.c
 *
 *  Created on: Sep 1, 2025
 *      Author: Xavier
 */


#include "FreeRTOS.h"
#include "task.h"

/* 模擬 HAL 的系統 tick counter */
volatile uint32_t uwTick = 0;

#if ( configUSE_TICK_HOOK == 1 )
void vApplicationTickHook(void)
{
    /* 每次 FreeRTOS SysTick 來時，自增 1ms */
    uwTick++;
}
#endif

/* 提供類似 HAL_GetTick 的功能 */
uint32_t GetSysTick(void)
{
    return uwTick;
}
