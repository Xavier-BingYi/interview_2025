################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/clock.c \
../Src/exti.c \
../Src/fmc.c \
../Src/gpio.c \
../Src/ltdc.c \
../Src/main.c \
../Src/mem_io.c \
../Src/rcc.c \
../Src/spi.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/usart.c 

OBJS += \
./Src/clock.o \
./Src/exti.o \
./Src/fmc.o \
./Src/gpio.o \
./Src/ltdc.o \
./Src/main.o \
./Src/mem_io.o \
./Src/rcc.o \
./Src/spi.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/usart.o 

C_DEPS += \
./Src/clock.d \
./Src/exti.d \
./Src/fmc.d \
./Src/gpio.d \
./Src/ltdc.d \
./Src/main.d \
./Src/mem_io.d \
./Src/rcc.d \
./Src/spi.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F429ZITx -DSTM32F4 -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/clock.cyclo ./Src/clock.d ./Src/clock.o ./Src/clock.su ./Src/exti.cyclo ./Src/exti.d ./Src/exti.o ./Src/exti.su ./Src/fmc.cyclo ./Src/fmc.d ./Src/fmc.o ./Src/fmc.su ./Src/gpio.cyclo ./Src/gpio.d ./Src/gpio.o ./Src/gpio.su ./Src/ltdc.cyclo ./Src/ltdc.d ./Src/ltdc.o ./Src/ltdc.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/mem_io.cyclo ./Src/mem_io.d ./Src/mem_io.o ./Src/mem_io.su ./Src/rcc.cyclo ./Src/rcc.d ./Src/rcc.o ./Src/rcc.su ./Src/spi.cyclo ./Src/spi.d ./Src/spi.o ./Src/spi.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/usart.cyclo ./Src/usart.d ./Src/usart.o ./Src/usart.su

.PHONY: clean-Src

