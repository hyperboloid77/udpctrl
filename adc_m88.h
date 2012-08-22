// Function prototypes to control the ADC of ATmega88
// Created by Ramunas Girdziusas, December 2010. The MIT Licence.

#ifndef ADC_M88_H
#define ADC_M88_H
void adc88_init(void);
void adc88_close(void);
void adc88_select_sensor(uint8_t num);
void adc88_enable_buffer(uint8_t num);
void adc88_disable_buffer(uint8_t num);
uint16_t adc88_readval(uint8_t num);
int8_t adc88_m2t(uint16_t measured_val);
void adc88_scan(char *tvalues);
void adc88_adjust(uint8_t adjust_flag);
#endif // ADC_M88_H
