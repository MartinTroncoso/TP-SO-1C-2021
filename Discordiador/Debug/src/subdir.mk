################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Comunicacion.c \
../src/Discordiador.c \
../src/Tripulantes.c 

OBJS += \
./src/Comunicacion.o \
./src/Discordiador.o \
./src/Tripulantes.o 

C_DEPS += \
./src/Comunicacion.d \
./src/Discordiador.d \
./src/Tripulantes.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/shared/src -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


