# --- PROJECT PARAMETERS ---
# Linux default. macOS: /dev/cu.usbmodem* | Windows: change PORT to COMx
MCU   = atmega328p
F_CPU = 16000000UL
BAUD  = 115200
PORT  = /dev/ttyACM0

# --- TOOLS DEFINITIONS ---
CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# --- COMPILATION FLAGS ---
# -Iinclude: tells the compiler to look for .h files in the include/ folder
CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Iinclude

# --- SOURCE FILES ---
# We specify the path for each .c file inside the src/ folder
SRCS = src/main.c src/lcd.c src/temp_sensor.c src/mcu_timer.c src/button.c

# --- MAIN TARGETS ---
all: main.hex main.bin

# 1. Link all source files from src/ into an ELF file
main.elf: $(SRCS)
	$(CC) $(CFLAGS) -o main.elf $(SRCS)

# 2. Extract the Intel HEX format for flashing
main.hex: main.elf
	$(OBJCOPY) -O ihex main.elf main.hex

# 3. Extract the raw binary footprint
main.bin: main.elf
	$(OBJCOPY) -O binary main.elf main.bin

# --- FLASHING ---
flash: main.hex
	$(AVRDUDE) -v -p $(MCU) -c arduino -P $(PORT) -b $(BAUD) -U flash:w:main.hex:i

# --- CLEANUP ---
# Using 'rm -f' for Linux and MacOS environments
# Windows: change 'rm -f' to 'del' in clean target
clean:
	rm -f main.elf main.hex main.bin