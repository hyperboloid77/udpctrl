1. Introduction
===============

This is the code for a remote server/client device that reads in the analog sensors of the environment and 
communicates to PC via UDP. The PC may also send the UDP packets to control five switches, which, in turn, can change 
the environment. The device uses the ethernet chip ENC28J60 with the ATmega88 microcontroller. 

2. System
=========

The structure of the system is shown in Fig. 1.

      (0)         (1)                            (3)                                (5)
    Sensors   ------------                  --------------                     --------------
    --------->| ATmega88 |        (2)       |   Router   |        (4)          | Linux PC   |
    <---------| ENC28j60 |<----Ethernet---->|            |<----Ethernet------> | (Ubuntu)   |
    Outputs   ------------                  --------------                     --------------

Figure 1. Network architecture of the remote server/client control device. In the LAN case 
          the router is not necessary.


2.0 Sensors and Outputs
-----------------------

All the four analog input lines ADC0-ADC3 of ATmega88 are sequentially scanned by using Single Conversion 
Mode. At present, the system reads in the four voltages that depend on the four 334-NTC103-RC thermistors, 
stores the voltages in a digital form, and converts them to the degrees in Celsius, see the function 
adc88_m2t(...) in adc_m88.c. In addition, the function

void adc88_adjust(uint8_t adjust_flag)

is created to control the switches by providing binary control signals on the pins of the Port D 
(PD0, PD1, PD3, PD4). The output PD7 is left free. The switches are connected to the electric heaters which 
raise the environment temperature if it falls below a specified threshold.

The NTC thermistors are such that their resistance decreases when the temperature rises. As a result, the 
change in the ADC values is of the opposite sign. The adjusting function uses two arrays:

uint16_t t_on[4]={450,450,450,450};
uint16_t t_off[4]={350,350,350,350}; 

Their elements are set by hand and indicate when the corresponding output should be turned on or off in order to keep the environment temperature above a specified threshold.

2.1 Device
----------

See the article by Guido Socher at http://www.linuxfocus.org/~katja/electronics/200606/article06061.shtml.
The implicit design criterion and the sole reason for the use of the ENC28J60 chip is the minimization of 
the number of the chip's pins.

2.2 Ethernet
------------

Standard 10BASE-T Ethernet cable.

2.3 Router
----------

Any standard router that you use to connect your PC to the internet is OK. One should write the device MAC, 
IP and the port number to the router's tables to obtain the chance of everything getting connected. 
The IP address for the device must be local.

2.4 Ethernet
------------

The system uses Internet's transport layer by sending the UDP packets. In order to minimize the packet 
collisions, or buffer overflow, it is good to use the device to respond to separate commands, 
as opposed to, say, a constant generation of the UDP packets. Occasional rebooting might still be necessary.

2.5 PC
------

The communication with the device is done from PC by running udpcom in the Linux shell, see
http://www.linuxfocus.org/~katja/electronics/200606/article06061.shtml.

3. Usage
========

3.1 Assemble all the hardware according to the scheme given by Guido Socher, see the link above. The scheme must be from the web article, not the one provided in the archived files, i.e. check if the clock signal is external: ENC28j60's Pin 3 (CLKout) should be connected to ATmega88's Pin 9 (PD5, XTAL1).

3.2 sudo aptitude install binutils, gcc-avr, avr-libc, avrdude.

3.3 Clone this folder, and type: make. The resulting main.hex may look to be of the size 15.3 KB, but this is
nothing to worry about as each symbol in it is the hexadecimal number which takes 4 bits, not a byte, and so 
the  actual code size to write is less than 8KB (should be about 6KB). This is the part where you begin 
to worry if you  plan to add some new libraries.

3.4 Set up the stk200 interface to connect the device to your PC and write sudo make install. 
This should write the file main.hex to the flash of ATmega88.

3.5 Turn the power of the device off, unplug the stk200 interface cable, plug in the ethernet cable, turn
the power on, and run the udpcom commands in several scenarios:

	Turn on the automatic temperature adjustment: udpcom password,a=1 <IP address>
	
	Turn off the automatic temperature adjustment: udpcom password,a=0 <IP address>

	Set the output PD7 to 1: udpcom password,s=5 <IP address>
	
	Set the output PD1 to 0: udpcom password,r=2 <IP address>
	
	Read the status of all the output pins: udpcom password,p <IP address>
	
	Read all the temperature measurements in Celsius: udpcom password,m <IP address>
	
<IP address> is the IP address of the router in the global setting, or any local IP address of the device if 
the LAN is configured without a router. The IP and MAC addresses, and the port number, all specified 
in main.c, should match the ones written in the router's table.

The automatic adjustment overrides the "s=x" and "r=y" commands, and that is the default setting. It must 
be turned off with "a=0" before issuing the "s=x" and "r=y" commands.
	
4. History, Credit
==================

This is the project completed by me and Saulius Rakauskas. He assembled the hardware and did all the fine work
with the network configuration, while I wrote the code. In essence, this is an adaptation of the work 
by Guido Socher:

"An AVR microcontroller based Ethernet device"

http://www.linuxfocus.org/~katja/electronics/200606/article06061.shtml

We added the analog-to-digit conversion, which resulted in the files adc_m88.c (h), and the adjustments in the 
files main.c and Makefile.

5. Licence
==========

GPL V2 for the modified Guido Socher's code. MIT for the ADC functions. Use the code at your own risk.
