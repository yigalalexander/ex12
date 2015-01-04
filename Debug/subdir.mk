################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../linux-scalability.c \
../mtmm.c 

OBJS += \
./linux-scalability.o \
./mtmm.o 

C_DEPS += \
./linux-scalability.d \
./mtmm.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -m32 -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -ansi -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


