# Makefile for STM32F103 (non HAL/SPL)

# Output filename
TARGET = OsExample

# Compiler & flags
CC      = arm-none-eabi-gcc
CFLAGS  = -mcpu=cortex-m3 -mthumb -O0 -g -Wall -ffreestanding -nostdlib \
			-ISys/inc
LDFLAGS = -T stm32f103.ld -nostdlib -Wl,--gc-sections

# Source files list
SRCS_C := main.c $(wildcard Sys/src/*.c)
SRCS_S := startup_stm32f103.s $(wildcard Sys/asm/*.s)
SRCS   := $(SRCS_C) $(SRCS_S)
OBJS   := $(SRCS:.c=.o)
OBJS   := $(OBJS:.s=.o)

# Default Target
all: $(TARGET).bin

# Compile C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM
%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Link ELF
$(TARGET).elf: $(OBJS) stm32f103.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

# Create .bin from .elf
$(TARGET).bin: $(TARGET).elf
	arm-none-eabi-objcopy -O binary $< $@

# Flash firmware into Blue Pill (file .bin)
flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $(TARGET).bin 0x08000000 verify reset exit"

# Delete unneeded files
clean:
	del /Q *.o *.elf *.bin 2>nul
	del /Q Sys\\src\\*.o 2>nul

.PHONY: all clean flash
