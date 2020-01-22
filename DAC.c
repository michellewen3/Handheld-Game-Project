// put implementations for functions, explain how it works
// put your names here, date
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

//DAC initialization for PB3-0
//PB3-0 are connected to resistor values of 
//1.5k,3k,6k,12k respectively in parallel

void DAC_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x02;
	delay++;
	GPIO_PORTB_DIR_R |= 0x0F; 
	GPIO_PORTB_DEN_R |= 0x0F; 	//PB3-0 are outputs from tm4c and inputs into DAC
}

//4-bit data (range from 0-15) is converted to n*3.3V/15
void DAC_Out(uint32_t data){
	GPIO_PORTB_DATA_R = data;		//output data from tm4c to DAC
}
