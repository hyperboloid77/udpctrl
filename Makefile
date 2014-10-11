# makefile, originally written by Guido Socher

# rewritten by Ramunas Girdziusas, December 2010.
# Removed compiling and linking of anything but eth_rem_dev.hex, which
# is now main.hex. Included linking of adc_m88.c and adc_m88.h. 
# Writing to flash is done strictly with avrdude and the stk200 interface.
# The setting lfuse:w:0x60:m as is essential as the clock is external: 
# ENC28j60 Pin 3 (CLKout) is connected to ATmega88 Pin 9 (PD5, XTAL1).

MCU=atmega88
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-g -mmcu=$(MCU) -Wall -Wstrict-prototypes -Os -mcall-prologues
PROG=avrdude
INTERFACE=stk200
PORT=/dev/parport0
#-------------------
all: main.hex
main.hex : main.out 
	$(OBJCOPY) -R .eeprom -O ihex main.out main.hex 
main.out : main.o ip_arp_udp.o enc28j60.o timeout.o adc_m88.o
	$(CC) $(CFLAGS) -o main.out -Wl,-Map,main.map main.o ip_arp_udp.o enc28j60.o timeout.o adc_m88.o
enc28j60.o : enc28j60.c avr_compat.h timeout.h enc28j60.h
	$(CC) $(CFLAGS) -Os -c enc28j60.c
ip_arp_udp.o : ip_arp_udp.c net.h avr_compat.h enc28j60.h
	$(CC) $(CFLAGS) -Os -c ip_arp_udp.c
main.o : main.c ip_arp_udp.h avr_compat.h enc28j60.h timeout.h net.h
	$(CC) $(CFLAGS) -Os -c main.c
timeout.o : timeout.c timeout.h 
	$(CC) $(CFLAGS) -Os -c timeout.c
adc_m88.o : adc_m88.c adc_m88.h 
	$(CC) $(CFLAGS) -Os -c adc_m88.c
#-------------------
help: 
	@echo "Type make or make install"	
#-------------------
install:
	$(PROG) -p $(MCU) -c $(INTERFACE) -P $(PORT) -U lfuse:w:0x60:m -U hfuse:w:0xdf:m -U flash:w:main.hex	
clean:
	rm -f *.o *.map *.out main.hex
#-------------------
