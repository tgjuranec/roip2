/*
 * dac.h
 *
 *  Created on: Feb 15, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_DAC_H_
#define APPS_DAC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <chip.h>
extern volatile uint8_t dac_dma_interrupt;

Status DAC_DMA_process(int16_t *buf, uint32_t len);
void DAC_init(uint32_t rate);


#ifdef __cplusplus
}
#endif

#endif /* APPS_DAC_H_ */
