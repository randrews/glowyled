# WinAVR cross-compiler toolchain is used here
CHIP=attiny85
MHZ=16000000
APP=led

##########

DUDEFLAGS = -p t85 -c usbasp
GCCFLAGS=-mmcu=${CHIP} -Os -DF_CPU=${MHZ} -std=gnu99 -Iusbdrv/ -Iws2812/ -I.

# Object files from vusb
VUSB = usbdrv/usbdrv.o usbdrv/usbdrvasm.o ws2812/light_ws2812.o

$(OBJECTS): usbconfig.h

.SUFFIXES: .elf .hex .bin

# By default, build the firmware and command-line client, but do not flash
all: ${APP}.hex

# With this, you can flash the firmware by just typing "make flash" on command-line
flash: ${APP}.hex
	avrdude $(DUDEFLAGS) -U flash:w:$<

fuses:
	avrdude $(DUDEFLAGS) -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m -B8

# Housekeeping if you want it
clean:
	rm -f *.o *.hex *.elf usbdrv/*.o ws2812/*.o

.elf.hex: ${APP}.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

.c.o:
	avr-gcc ${GCCFLAGS} -c $< -o $@

.c:

${APP}.elf: ${APP}.o ${VUSB}
	avr-gcc ${GCCFLAGS} -o $@ *.o ${VUSB}

# From assembler source to .o object file
%.o: %.S
	avr-gcc ${GCCFLAGS} -x assembler-with-cpp -c $< -o $@
