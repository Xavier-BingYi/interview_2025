/*
 * FreeRTOSConfig.h
 *
 *  Created on: Sep 1, 2025
 *      Author: Xavier
 */

#ifndef FREERTOS_FREERTOSCONFIG_H_
#define FREERTOS_FREERTOSCONFIG_H_

#include <stdint.h>
extern uint32_t SystemCoreClock;

/*-----------------------------------------------------------
 * Kernel basic
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 7 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 16 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/*-----------------------------------------------------------
 * Hooks（先全部關掉，之後需要再開）
 *----------------------------------------------------------*/
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     1   /* 臨時橋接用；改 TIM6 後可再關回 0 */
#define configUSE_MALLOC_FAILED_HOOK            0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/*-----------------------------------------------------------
 * 同步機制 / 其他功能
 *----------------------------------------------------------*/
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TRACE_FACILITY                0

/* 事件群組 / 計時器 */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )
#define configUSE_QUEUE_SETS                    1

/* Stream/Message Buffer（可用時再開；核心 .c 你未加入也沒關係） */
#define configUSE_STREAM_BUFFERS                1
#define configUSE_MESSAGE_BUFFERS               1

/*-----------------------------------------------------------
 * 可移植層相關（STM32F4 = 4 個優先權位）
 *----------------------------------------------------------*/
#define configPRIO_BITS                         4

/* 不要改這兩個 library 值，只調整數字 15 / 5 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5  /* 5(含)以上的 IRQ 不可呼叫 FreeRTOS API */

#define configKERNEL_INTERRUPT_PRIORITY  ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* 斷言 */
#define configASSERT( x )  if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/*-----------------------------------------------------------
 * 運行時統計（不用就關）
 *----------------------------------------------------------*/
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/*-----------------------------------------------------------
 * 映射到 CMSIS 例外處理名稱
 *----------------------------------------------------------*/
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/*-----------------------------------------------------------
 * 可選：記憶體分配模式（靜態/動態）
 *----------------------------------------------------------*/
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/*-----------------------------------------------------------
 * FreeRTOS API 開關（至少要這兩個）
*----------------------------------------------------------*/
#define INCLUDE_vTaskDelay        1
#define INCLUDE_vTaskDelayUntil   1

#endif /* FREERTOS_FREERTOSCONFIG_H_ */
