// Functions to control the ADC of ATmega88
// Created by Ramunas Girdziusas, December 2010. The MIT Licence.

#include <avr/io.h>
#include <inttypes.h>

#ifndef F_CPU
#define F_CPU 8000000UL // 8 MHz
#endif // F_CPU

#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

void adc88_init(void) {
   
   //Turn on ADC     
   PRR &= ~(1<<PRADC);
   ADCSRA|=(1<<ADEN);

   //Internal reference voltage with C on pin Aref
   ADMUX |= (1<<REFS1)|(1<<REFS0);                   
     
   //Set the division factor between the system clock and the ADC input clock
   //to 64. ADC clock is then 8Mhz/128=62.5KHz. Takes 13-25 ADC cycles to get
   //one measurement, which is about 0.2 ms.
         
   ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);    
} 

void adc88_close(void) {

   //Turn off ADC
   ADCSRA&=~(1<<ADEN);     
   PRR |= (1<<PRADC);
}

void adc88_select_sensor(uint8_t num) {
   
   switch(num)
   {
     case 0: 
        //Analog input on pin ADC0: MUX=0000
        ADMUX &= (~(1<<MUX3))&(~(1<<MUX2))&(~(1<<MUX1))&(~(1<<MUX0));
        break;
     case 1:
        //Analog input on pin ADC1: MUX=0001
        ADMUX &= (~(1<<MUX3))&(~(1<<MUX2))&(~(1<<MUX1));
        ADMUX |= (1<<MUX0);
        break;
     case 2:
        //Analog input on pin ADC2: MUX=0010
        ADMUX &= (~(1<<MUX3))&(~(1<<MUX2))&(~(1<<MUX0));
        ADMUX |= (1<<MUX1);
        break;
     case 3:
        //Analog input on pin ADC3: MUX=0011
        ADMUX &= (~(1<<MUX3))&(~(1<<MUX2));
        ADMUX |= (1<<MUX1)|(1<<MUX0);
        break; 
   }  
}

void adc88_enable_buffer(uint8_t num) {
   
   switch(num)
   {
     case 0: 
        //Enable digital input buffer on ADC0
        DIDR0 &= ~(1<<ADC0D);
        break;
     case 1:
        //Enable digital input buffer on ADC1
        DIDR0 &= ~(1<<ADC1D);
        break;
     case 2:
        //Enable digital input buffer on ADC2
        DIDR0 &= ~(1<<ADC2D);
        break;
     case 3:
        //Enable digital input buffer on ADC3
        DIDR0 &= ~(1<<ADC3D); 
        break; 
   }  
}          

void adc88_disable_buffer(uint8_t num) {
   
   switch(num)
   {
     case 0: 
        //Disable digital input buffer on ADC0
        DIDR0 |= (1<<ADC0D);
        break;
     case 1:
        //Disable digital input buffer on ADC1
        DIDR0 |= (1<<ADC1D);
        break;
     case 2:
        //Disable digital input buffer on ADC2
        DIDR0 |= (1<<ADC2D);
        break;
     case 3:
        //Disable digital input buffer on ADC3
        DIDR0 |= (1<<ADC3D);
        break; 
   }  
}          
   
uint16_t adc88_readval(uint8_t num) {  
   
   uint16_t value=0;     
   
   adc88_init();
   
   adc88_select_sensor(num);
         
   adc88_enable_buffer(num);
   
   //Begin the conversion
   ADCSRA|=(1<<ADSC);
   //Wait until it ends
   while((ADCSRA & (1<<ADSC))!=0);
   
   value=ADC;
   
   adc88_disable_buffer(num);
   
   adc88_close();
   
   return value;               
}

int8_t adc88_m2t(uint16_t measured_val){
// Convert the ADC measurements to the temperatures in Celsius
  
   int8_t display_val;
  
   if(measured_val >550)                           display_val=-5;
   else if(measured_val >450 && measured_val <=550) display_val=0;
   else if(measured_val >440 && measured_val <=450) display_val=1;
   else if(measured_val >430 && measured_val <=440) display_val=2;
   else if(measured_val >420 && measured_val <=430) display_val=3;
   else if(measured_val >410 && measured_val <=420) display_val=4;
   else if(measured_val >400 && measured_val <=410) display_val=5;
   else if(measured_val >300 && measured_val <=400) display_val=10;
   else if(measured_val >250 && measured_val <=300) display_val=15;
   else if(measured_val >210 && measured_val <=250) display_val=20;
   else if(measured_val >190 && measured_val <=210) display_val=25;
   else if(measured_val >150 && measured_val <=190) display_val=30;
   else if(measured_val <=150) display_val=35;
   else display_val=-127;        
   return display_val;
}

void adc88_scan(char *tvalues){
  
   int8_t ts[4];
   uint16_t foo;            
  
   uint16_t i;
   for(i = 0; i < 4; i++) 
   {  
      foo=adc88_readval(i);
      ts[i]=adc88_m2t(foo);
      _delay_ms(2);
   }
   foo=sprintf(tvalues,"%d, %d, %d, %d\n",ts[0],ts[1],ts[2],ts[3]);
}

void adc88_adjust(uint8_t adjust_flag){

   if(adjust_flag){        
      uint16_t tvals[4]={0,0,0,0};
      uint16_t t_on[4]={450,450,450,450};
      uint16_t t_off[4]={350,350,350,350}; 

      uint8_t term_num;
      for(term_num = 0; term_num < 3; term_num++){ 
         tvals[term_num]=adc88_readval(term_num);
  
         if(tvals[term_num] > t_on[term_num]){
         //pins "on"
            switch (term_num){
            case 0:     
               PORTD|= (1<<PD0);
               break;
            case 1:     
               PORTD|= (1<<PD1);
               break;
            case 2:     
               PORTD|= (1<<PD3);
               break;
            case 3:     
               PORTD|= (1<<PD4);
               break;
            }
         }
         else if (tvals[term_num] < t_off[term_num]){
            //pins "off"
            switch (term_num){
            case 0:     
               PORTD&=~(1<<PD0);
               break;
            case 1:     
               PORTD&=~(1<<PD1);
               break;
            case 2:     
               PORTD&=~(1<<PD3);
               break;
            case 3:     
               PORTD&=~(1<<PD4);
               break;
            }                                
         }    
         _delay_ms(2);
      }
   }
}
