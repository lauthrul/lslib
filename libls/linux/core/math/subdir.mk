################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
$(SRC_DIR)/core/math/math.cpp 

HEADERS += \
$(SRC_DIR)/core/math/math.h 

OBJS += \
./core/math/math.o 

CPP_DEPS += \
./core/math/math.d 


# Each subdirectory must supply rules for building sources it contributes
core/math/%.o: $(SRC_DIR)/core/math/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GCPP) $(INCLUDE_PATH) $(DEFINES) -O0 -g3 -Wall -c -fmessage-length=0 -fpermissive -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


