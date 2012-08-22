/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * Ethernet remote device and sensor
 *
 * Title: Microchip ENC28J60 Ethernet Interface Driver
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/

/* Modified by Ramunas Girdziusas, December 2010.
   * Simplified the while(1) loop and applied the ADC functions, i.e. 
   * #include "adc_m88.h", and mostly lines from 190 to 270. 
*/

#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ip_arp_udp.h"
#include "enc28j60.h"
#include "timeout.h"
#include "avr_compat.h"
#include "net.h"
#include "adc_m88.h"

//#include <avr/interrupt.h>
//#include <avr/wdt.h>


#ifndef F_CPU
#define F_CPU 8000000UL // 8 MHz
#endif // F_CPU

#include <util/delay.h>
//find /usr/ -print | xargs grep '_delay_ms' > out.txt

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x00,0x55,0x58,0x10,0x11,0x07};
static uint8_t myip[4] = {192,168,10,150};
static uint16_t myport =1200; // listen port for udp

#define BUFFER_SIZE 250
static uint8_t buf[BUFFER_SIZE+1];

// the password string (only the first 5 char checked):
static char password[]="password";

uint8_t verify_password(char *str);


int main(void){
	        
        uint16_t plen;
        uint8_t i=0;
        uint8_t cmd_pos=0;
        uint8_t payloadlen=0;
        char str[50];
        char cmdval;
        
	    uint8_t adjust_flag=1;
        uint8_t pd[5], foo;	

        // set the clock speed to 8MHz
        // set the clock prescaler. First write CLKPCE to enable setting of clock the
        // next four instructions.
        CLKPR=(1<<CLKPCE);
        CLKPR=0; // 8 MHZ
        _delay_ms(10);
        
        /* enable PB0, reset as output */
        DDRB|= (1<<DDB0);

        /* enable PD2/INT0, as input */
        DDRD&= ~(1<<DDD2);

        /* set output to gnd, reset the ethernet chip */
        PORTB &= ~(1<<PB0);
        _delay_ms(10);
        /* set output to Vcc, reset inactive */
        PORTB|= (1<<PB0);
        _delay_ms(200);
        
        /*initialize enc28j60*/
        enc28j60Init(mymac);
        _delay_ms(20);
        
        // LED
        /* enable PB1, LED as output */
        DDRB|= (1<<DDB1);

        /* set output to Vcc, LED off */
        PORTB|= (1<<PB1);

        //make PORT D to be writable
        DDRD|= (1<<DDD7);
        DDRD|= (1<<DDD4);
        DDRD|= (1<<DDD3);
        DDRD|= (1<<DDD1);
        DDRD|= (1<<DDD0);

	    //pins "off"
        PORTD&=~(1<<PD0);
        PORTD&=~(1<<PD1);
        PORTD&=~(1<<PD3);
        PORTD&=~(1<<PD4);
	    PORTD&=~(1<<PD7);	
        
        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
        _delay_ms(20);
        
        /* set output to GND, red LED on */
        PORTB &= ~(1<<PB1);
        i=1;

        //init the ethernet/ip layer:
        init_ip_arp_udp(mymac,myip);
        
        while(1){
                
                adc88_adjust(adjust_flag);

                // get the next new packet:
                plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

                /*plen will ne unequal to zero if there is a valid 
                 * packet (without crc error) */
                if(plen==0){
                        continue;
                }
                // led----------
                if (i){
                        /* set output to Vcc, LED off */
                        PORTB|= (1<<PB1);
                        i=0;
                }else{
                        /* set output to GND, LED on */
                        PORTB &= ~(1<<PB1);
                        i=1;
                }
                        
                // arp is broadcast if unknown but a host may also
                // verify the mac address by sending it to 
                // a unicast address.
                if(eth_type_is_arp_and_my_ip(buf,plen)){
                        make_arp_answer_from_request(buf,plen);
                        continue;
                }
                // check if ip packets (icmp or udp) are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                        continue;
                }
                
                if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
                        // a ping packet, let's send pong
                        make_echo_reply_from_request(buf,plen);
                        continue;
                }
                // we listen on port 1200=0x4B0
                if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==4&&buf[UDP_DST_PORT_L_P]==0xb0){
                        payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
                        // you must sent a string starting with v
                        // e.g udpcom version 10.0.0.24
                        if (verify_password((char *)&(buf[UDP_DATA_P]))){
                                // find the first comma which indicates 
                                // the start of a command:
                                cmd_pos=0;
                                while(cmd_pos<payloadlen){
                                        cmd_pos++;
                                        if (buf[UDP_DATA_P+cmd_pos]==','){
                                                cmd_pos++; // put on start of cmd
                                                break;
                                        }
                                }
                                
				
				switch (buf[UDP_DATA_P+cmd_pos]){
					// a=1 turns the temperature adjusting on
					// a=0 turns it off 
					case 'a':
						cmdval=buf[UDP_DATA_P+cmd_pos+2];
						switch (cmdval){
                                                	case '0':        
                                                        	adjust_flag=0;
                                                        	strcpy(str,"Adjusting is off");
                                                        	break;
							case '1':        
                                                        	adjust_flag=1;
                                                        	strcpy(str,"Adjusting is on");
                                                        	break;
						}
						break;
					// Turning on the switch, e.g. s=5 sets the pin PD7 to "on"
					case 's':
 						cmdval=buf[UDP_DATA_P+cmd_pos+2];
						switch (cmdval){
                            case '1':        
                                                        	PORTD|= (1<<PD0);
                                                        	strcpy(str,"Switch 1 is on");
                                                        	break;
							case '2':        
                                                        	PORTD|= (1<<PD1);
                                                        	strcpy(str,"Switch 2 is on");
                                                        	break;
							case '3':        
                                                        	PORTD|= (1<<PD3);
                                                        	strcpy(str,"Switch 3 is on");
                                                        	break;
							case '4':        
                                                        	PORTD|= (1<<PD4);
                                                        	strcpy(str,"Switch 4 is on");
                                                        	break;
							case '5':        
                                                        	PORTD|= (1<<PD7);
                                                        	strcpy(str,"Switch 5 is on");
                                                        	break;
						}
						break;
					// Turning off the switch, e.g. r=5 sets the pin PD7 to "off"
                   	case 'r':
 						cmdval=buf[UDP_DATA_P+cmd_pos+2];
						switch (cmdval){
                            case '1':        
                                                        	PORTD&= ~(1<<PD0);
                                                        	strcpy(str,"Switch 1 is off");
                                                        	break;
							case '2':        
                                                        	PORTD&= ~(1<<PD1);
                                                        	strcpy(str,"Switch 2 is off");
                                                        	break;
							case '3':        
                                                        	PORTD&= ~(1<<PD3);
                                                        	strcpy(str,"Switch 3 is off");
                                                        	break;
							case '4':        
                                                        	PORTD&= ~(1<<PD4);
                                                        	strcpy(str,"Switch 4 is off");
                                                        	break;
							case '5':        
                                                        	PORTD&= ~(1<<PD7);
                                                        	strcpy(str,"Switch 5 is off");
                                                        	break;
						}
						break;
					// Get the pin values
					case 'p':
						//See /usr/lib/avr/include/avr/iom88p.h 
						pd[0]=(PORTD & (1<<PD0));
						pd[1]=((PORTD & (1<<PD1)))>>1;
						pd[2]=((PORTD & (1<<PD3)))>>3;
						pd[3]=((PORTD & (1<<PD4)))>>4;
						pd[4]=((PORTD & (1<<PD7)))>>7;	
						foo=sprintf(str,"%d, %d, %d, %d, %d\n",pd[0],pd[1],pd[2],pd[3],pd[4]);      
                                   		break;
					// Get all the temperatures
                                  	case 'm':
                                   		adc88_scan(str);
                                   		break;
                                        default :
                                   		strcpy(str,"e=no_such_cmd");
                                                break;     
				}

                        }
                        
                        else {
                                strcpy(str,"e=invalid_pw");
                        }
                        
                        make_udp_reply_from_request(buf,str,strlen(str),myport);
                        
                }
        }//end of while(1)
        return (0);
}


// 
uint8_t verify_password(char *str)
{
        // the first characters of the received string are
        // a simple password/cookie:
        if (strncmp(password,str,5)==0){
                return(1);
        }
        return(0);
}
