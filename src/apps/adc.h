/*
 * adc.h
 *
 *  Created on: 18. 1. 2014.
 *      Author: djuka
 */

#ifndef ADC_H_
#define ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <chip.h>
//1.declaration of the function in library
typedef void (*adc_finished)(int16_t *,int, void *);



#define BLOCK_SIZE						160*4

//2.entry for a function in library
void ADC_init(uint8_t adc_port, uint8_t adc_pin,
		ADC_CHANNEL_T ch, uint32_t rate,
		adc_finished fn, void *arg);
inline void ADC_start(ADC_CHANNEL_T ch);
inline void ADC_stop(ADC_CHANNEL_T ch);
volatile extern uint8_t n_conv_finished;
extern int16_t *block_result;
#ifdef __cplusplus
}
#endif

#endif /* ADC_H_ */
