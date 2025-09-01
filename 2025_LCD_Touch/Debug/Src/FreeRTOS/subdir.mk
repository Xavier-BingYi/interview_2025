################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/FreeRTOS/event_groups.c \
../Src/FreeRTOS/heap_4.c \
../Src/FreeRTOS/hooks.c \
../Src/FreeRTOS/list.c \
../Src/FreeRTOS/queue.c \
../Src/FreeRTOS/tasks.c \
../Src/FreeRTOS/timers.c 

OBJS += \
./Src/FreeRTOS/event_groups.o \
./Src/FreeRTOS/heap_4.o \
./Src/FreeRTOS/hooks.o \
./Src/FreeRTOS/list.o \
./Src/FreeRTOS/queue.o \
./Src/FreeRTOS/tasks.o \
./Src/FreeRTOS/timers.o 

C_DEPS += \
./Src/FreeRTOS/event_groups.d \
./Src/FreeRTOS/heap_4.d \
./Src/FreeRTOS/hooks.d \
./Src/FreeRTOS/list.d \
./Src/FreeRTOS/queue.d \
./Src/FreeRTOS/tasks.d \
./Src/FreeRTOS/timers.d 


# Each subdirectory must supply rules for building sources it contributes
Src/FreeRTOS/%.o Src/FreeRTOS/%.su Src/FreeRTOS/%.cyclo: ../Src/FreeRTOS/%.c Src/FreeRTOS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F429ZITx -DSTM32F4 -c -I../Inc -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Src/FreeRTOS/include" -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Src/FreeRTOS/portable/GCC/ARM_CM4F" -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Inc/FreeRTOS" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src-2f-FreeRTOS

clean-Src-2f-FreeRTOS:
	-$(RM) ./Src/FreeRTOS/event_groups.cyclo ./Src/FreeRTOS/event_groups.d ./Src/FreeRTOS/event_groups.o ./Src/FreeRTOS/event_groups.su ./Src/FreeRTOS/heap_4.cyclo ./Src/FreeRTOS/heap_4.d ./Src/FreeRTOS/heap_4.o ./Src/FreeRTOS/heap_4.su ./Src/FreeRTOS/hooks.cyclo ./Src/FreeRTOS/hooks.d ./Src/FreeRTOS/hooks.o ./Src/FreeRTOS/hooks.su ./Src/FreeRTOS/list.cyclo ./Src/FreeRTOS/list.d ./Src/FreeRTOS/list.o ./Src/FreeRTOS/list.su ./Src/FreeRTOS/queue.cyclo ./Src/FreeRTOS/queue.d ./Src/FreeRTOS/queue.o ./Src/FreeRTOS/queue.su ./Src/FreeRTOS/tasks.cyclo ./Src/FreeRTOS/tasks.d ./Src/FreeRTOS/tasks.o ./Src/FreeRTOS/tasks.su ./Src/FreeRTOS/timers.cyclo ./Src/FreeRTOS/timers.d ./Src/FreeRTOS/timers.o ./Src/FreeRTOS/timers.su

.PHONY: clean-Src-2f-FreeRTOS

