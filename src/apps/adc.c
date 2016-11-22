/*
 * adc.c
 *
 *  Created on: 18. 1. 2014.
 *      Author: djuka
 */



#include <chip.h>
#include "adc.h"




adc_finished send_samples;
void *arg_callback;
//two buffers
//one for filling from adc the other for lwip process
int16_t x1[BLOCK_SIZE];
int16_t x2[BLOCK_SIZE];
int16_t *block_result;
volatile int16_t *block_fill;
volatile uint8_t n_conv_finished;

void ADC_init(uint8_t adc_port, uint8_t adc_pin,
		ADC_CHANNEL_T ch, uint32_t rate,
		adc_finished fn, void *arg){
	NVIC_DisableIRQ(ADC_IRQn);
	__disable_irq();
	ADC_CLOCK_SETUP_T ADCSetup;
	ADCSetup.adcRate = rate;
	ADCSetup.bitsAccuracy = 12;
	ADCSetup.burstMode = false;
	//set GPIO direction
	Chip_GPIO_Init(LPC_GPIO);
	Chip_GPIO_WriteDirBit(LPC_GPIO,adc_port,adc_pin,false);
	//set input output  function
	Chip_IOCON_PinMux(LPC_IOCON, adc_port, adc_pin, IOCON_MODE_PULLDOWN , IOCON_FUNC1);
	//turn on ADC subsystem
	LPC_SYSCTL->PCONP |= (1 << 12);
	//setting ADC subsystem
	Chip_ADC_Init(LPC_ADC,&ADCSetup);					//init
	Chip_ADC_SetSampleRate(LPC_ADC,&ADCSetup, rate);
	Chip_ADC_EnableChannel(LPC_ADC,ch,ENABLE);
	Chip_ADC_Int_SetChannelCmd(LPC_ADC,ch,ENABLE);	//enable channel
	Chip_ADC_SetStartMode(LPC_ADC,ADC_NO_START,ADC_TRIGGERMODE_FALLING);
	send_samples = fn;
	arg_callback = arg;
	block_fill = &x1[0];
	block_result = &x2[0];
	__enable_irq();
	return;
}

inline void ADC_start(ADC_CHANNEL_T ch){
	__disable_irq();
	Chip_ADC_Int_SetChannelCmd(LPC_ADC,ch,ENABLE);	//enable channel
	Chip_ADC_SetStartMode(LPC_ADC,ADC_START_NOW,ADC_TRIGGERMODE_FALLING);
	__enable_irq();
	NVIC_EnableIRQ(ADC_IRQn);
}

inline void ADC_stop(ADC_CHANNEL_T ch){
	NVIC_DisableIRQ(ADC_IRQn);
	__disable_irq();
	Chip_ADC_Int_SetChannelCmd(LPC_ADC,ch,DISABLE);	//enable channel
	__enable_irq();
}

void ADC_error(){
	ADC_stop(ADC_CH2);
}




void ADC_IRQHandler(){
	ADC_start(ADC_CH2);
	__disable_irq();
	static uint32_t nSamples = 0;
	uint16_t uaccept;
	uaccept = LPC_ADC->DR[ADC_CH2];
	/*if (!ADC_DR_DONE(uaccept)) {
		return;
	}*/
	block_fill[nSamples] = (int16_t )(((int16_t)(uaccept & 0xFFF0) - 0x8000));	//convert DC to AC signal
	nSamples++;
	if(nSamples >= BLOCK_SIZE){		//BUFFER is full -> inform the main loop
		//exchange the pointers fill-result
		int16_t *res = block_result;
		block_result = block_fill;
		block_fill = res;
		//send to a callback function
		if(send_samples != NULL){	//if callback function is provided
			send_samples(block_result,nSamples+1,arg_callback);
		}
		//inform the main loop
		n_conv_finished++;
		nSamples = 0;
	}
	__enable_irq();
	return;
}
