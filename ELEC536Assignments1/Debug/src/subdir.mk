################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).cpp \
../src/ELEC536Assignment1.cpp 

OBJS += \
./src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).o \
./src/ELEC536Assignment1.o 

CPP_DEPS += \
./src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).d \
./src/ELEC536Assignment1.d 


# Each subdirectory must supply rules for building sources it contributes
src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).o: ../src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include/opencv -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/ELEC536Assignment1 (lcd-prototype's conflicted copy 2011-02-08).d" -MT"src/ELEC536Assignment1\ (lcd-prototype's\ conflicted\ copy\ 2011-02-08).d" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include/opencv -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


