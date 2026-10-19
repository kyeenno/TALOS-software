################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/kyeenno/STM32Cube_FW_F4_V1.28.0/Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.c \
../Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.c \
../Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.c 

OBJS += \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.o \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.o \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.o 

C_DEPS += \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.d \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.d \
./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.o: /home/kyeenno/STM32Cube_FW_F4_V1.28.0/Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.c Drivers/BSP/STM32F411E-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32F411xE -DUSE_STM32F411E_DISCO -DUSE_HAL_DRIVER -c -I../../Inc -I../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../Drivers/BSP/Components/l3gd20 -I../../../../../../../Drivers/BSP/Components/lsm303dlhc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../../Drivers/BSP/STM32F411E-Discovery -I../../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F411E-Discovery/%.o Drivers/BSP/STM32F411E-Discovery/%.su Drivers/BSP/STM32F411E-Discovery/%.cyclo: ../Drivers/BSP/STM32F411E-Discovery/%.c Drivers/BSP/STM32F411E-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32F411xE -DUSE_STM32F411E_DISCO -DUSE_HAL_DRIVER -c -I../../Inc -I../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../Drivers/BSP/Components/l3gd20 -I../../../../../../../Drivers/BSP/Components/lsm303dlhc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../../Drivers/BSP/STM32F411E-Discovery -I../../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32F411E-2d-Discovery

clean-Drivers-2f-BSP-2f-STM32F411E-2d-Discovery:
	-$(RM) ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.cyclo ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.d ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.o ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery.su ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.cyclo ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.d ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.o ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_accelerometer.su ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.cyclo ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.d ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.o ./Drivers/BSP/STM32F411E-Discovery/stm32f411e_discovery_gyroscope.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32F411E-2d-Discovery

