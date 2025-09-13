################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/FreeRTOS/portable/GCC/ARM_CM4F/port.c 

OBJS += \
./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.o 

C_DEPS += \
./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.d 


# Each subdirectory must supply rules for building sources it contributes
Src/FreeRTOS/portable/GCC/ARM_CM4F/%.o Src/FreeRTOS/portable/GCC/ARM_CM4F/%.su Src/FreeRTOS/portable/GCC/ARM_CM4F/%.cyclo: ../Src/FreeRTOS/portable/GCC/ARM_CM4F/%.c Src/FreeRTOS/portable/GCC/ARM_CM4F/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F429ZITx -DSTM32F4 -c -I../Inc -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Src/FreeRTOS/include" -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Src/FreeRTOS/portable/GCC/ARM_CM4F" -I"C:/Users/Xavier/Desktop/mine/SourceTree/interview_2025/2025_LCD_Touch/Inc/FreeRTOS" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM4F

clean-Src-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM4F:
	-$(RM) ./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.cyclo ./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.d ./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.o ./Src/FreeRTOS/portable/GCC/ARM_CM4F/port.su

.PHONY: clean-Src-2f-FreeRTOS-2f-portable-2f-GCC-2f-ARM_CM4F

