################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
hello_spi.elf.size \

ELFCHECK += \
hello_spi.elf.elfcheck \


# All Target
all: hello_spi.elf secondary-outputs

# Tool invocations
hello_spi.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo Building target: $@
	@echo Invoking: MicroBlaze gcc linker
	mb-gcc -Wl,-T -Wl,../src/lscript.ld -L../../sp605_bist_bsp_0/microblaze_0/lib -mxl-pattern-compare -mcpu=v8.20.b -mno-xl-soft-mul -o"hello_spi.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@
	@echo ' '

hello_spi.elf.size: hello_spi.elf
	@echo Invoking: MicroBlaze Print Size
	mb-size hello_spi.elf  |tee "hello_spi.elf.size"
	@echo Finished building: $@
	@echo ' '

hello_spi.elf.elfcheck: hello_spi.elf
	@echo Invoking: Xilinx ELF Check
	elfcheck hello_spi.elf -hw ../../hw_platform_0/system.xml -pe microblaze_0  |tee "hello_spi.elf.elfcheck"
	@echo Finished building: $@
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES)$(ELFSIZE)$(ELFCHECK)$(S_UPPER_DEPS) hello_spi.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE) $(ELFCHECK)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
