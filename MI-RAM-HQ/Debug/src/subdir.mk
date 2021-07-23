################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MI-RAM-HQ.c \
../src/Mapa.c \
../src/admin_memoria.c \
../src/paginacion.c \
../src/segmentacion.c 

OBJS += \
./src/MI-RAM-HQ.o \
./src/Mapa.o \
./src/admin_memoria.o \
./src/paginacion.o \
./src/segmentacion.o 

C_DEPS += \
./src/MI-RAM-HQ.d \
./src/Mapa.d \
./src/admin_memoria.d \
./src/paginacion.d \
./src/segmentacion.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/shared/src -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


