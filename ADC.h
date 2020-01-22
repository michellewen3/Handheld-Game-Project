// put prototypes for public functions, explain what it does
// put your names here, date
#include <stdint.h>
#ifndef ADC_H
#define ADC_H

void ADC_Init(void);

uint32_t ADC_In(void);
	
uint32_t Convert(uint32_t input);

#endif
