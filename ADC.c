// ADC.c for slidepot
// put your names here, date
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

//ADC initialization for PD2 (slidepot)
void ADC_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x08;
	while ((SYSCTL_PRGPIO_R&0x08) ==0){};
	GPIO_PORTD_DIR_R &= ~0x04;
	GPIO_PORTD_AFSEL_R |= 0x04;				//enable alternate functions on PD2
	GPIO_PORTD_DEN_R &= ~0x04;
	GPIO_PORTD_AMSEL_R |= 0x04;				//enabling analog function for PD2 
	SYSCTL_RCGCADC_R |= 0x01;					//activating ADC0
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	ADC0_PC_R = 0x01;									//max speed is 125k samples/sec
  ADC0_SSPRI_R = 0x0123;						//Seq3 is highest priority
	ADC0_ACTSS_R &= ~0x0008;					//disables sample Seq3
	ADC0_EMUX_R &= ~0xF000;						//Seq3 is software trigger
	ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+5;		//set channel AIN5 (PD2)
	ADC0_SSCTL3_R = 0x0006;						//no TS0 D0, yes IE0 END0	
	ADC0_IM_R &= ~0x0008;							//disable SS3 interrupts
	ADC0_ACTSS_R |= 0x0008;						//enable sample Seq3
	ADC0_SAC_R = 0x06;
}

//Samples the ADC when called
uint32_t ADC_In(void){  
  uint32_t data;
	ADC0_PSSI_R = 0x0008;							//start a sample collection
	while ((ADC0_RIS_R&0x08) == 0){};	//wait until flag is set/ready
	data = ADC0_SSFIFO3_R&0xFFF;			//read sample
	ADC0_ISC_R = 0x0008;							//clear flag indirectly using ISC register
  return data;
}

//Converts from 0-4095 to distance of 0-2000
uint32_t Convert(uint32_t input){
   return (((1805*input)/4096) + 179);
}	
