# See file COPYING.

PROJECT=uartttest
DEPS=uart.h main.h Makefile
SOURCES=main.c uart.c
CC=avr-gcc
LD=avr-ld
OBJCOPY=avr-objcopy
MMCU=atmega328p
SERIAL_DEV ?= /dev/ttyACM0


#AVRBINDIR=/usr/avr/bin/

AVRDUDECMD=avrdude -p m328p -P $(SERIAL_DEV) -b 115200 -c arduino

CFLAGS=-mmcu=$(MMCU) -Os -Wl,--relax -fno-tree-switch-conversion -frename-registers -g -Wall -W -pipe -flto -flto-partition=none -fwhole-program -std=gnu99 -Wno-main

all: $(PROJECT).out serialtest

$(PROJECT).hex: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).bin: $(PROJECT).out
	$(AVRBINDIR)$(OBJCOPY) -j .text -j .data -O binary $(PROJECT).out $(PROJECT).bin

$(PROJECT).out: $(SOURCES) $(DEPS)
	$(AVRBINDIR)$(CC)  $(CFLAGS) -I./ -o $(PROJECT).out $(SOURCES)
	$(AVRBINDIR)avr-size $(PROJECT).out

asm: $(SOURCES) $(DEPS)
	$(AVRBINDIR)$(CC) $(CFLAGS) -S  -I./ -o $(PROJECT).s $(SOURCES)


program: $(PROJECT).hex
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:w:$(PROJECT).hex


clean:
	-rm -f $(PROJECT).bin
	-rm -f $(PROJECT).out
	-rm -f $(PROJECT).hex
	-rm -f $(PROJECT).s
	-rm -f *.o
	-rm -f serialtest

backup:
	$(AVRBINDIR)$(AVRDUDECMD) -U flash:r:backup.bin:r

objdump: $(PROJECT).out
	$(AVRBINDIR)avr-objdump -xdC $(PROJECT).out | less

serialtest: serialtest.c
	gcc -W -Wall -Os -std=gnu99 -o serialtest serialtest.c
