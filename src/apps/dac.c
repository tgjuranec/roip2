/*
 * @brief DAC example
 * This example show how to use the D/A Conversion in 2 modes: Polling and DMA
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "chip.h"



/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

uint16_t dac_buff1[160*4];
uint16_t dac_buff2[160*4];
uint16_t *dac_buffin;
uint16_t *dac_buffout;
static volatile uint8_t dac_datawaitfullflag, dac_dataprocflag;
/* Work variables */
static volatile uint8_t dmaChannelNum;
static volatile uint8_t  Interrupt_Continue_Flag;


/* DAC sample rate request time */
#define DAC_TIMEOUT 0x3FF

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/
/**
 * @brief	Accept new buffer for DAC-ing
 * @return	SUCCESS if accepting buffer is empty
 * 			ERROR if previous data are still in accepting buffer, and they are overwritten
 */

Status DAC_DMA_process(int16_t *buf, uint32_t len)
{
	uint16_t val;
	uint16_t i = 0;
	if(len > 160*4) return ERROR;
	for(i = 0; i < len; i++){
		val = ((uint16_t)(buf[i]+0x8000) & 0xFFC0);
		dac_buffout[i] = (uint16_t) (val);
	}
	if(dac_dataprocflag == 0){//we can start with sending data immediately
		NVIC_EnableIRQ(DMA_IRQn);
		/* Get the free channel for DMA transfer */
		dmaChannelNum = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_DAC);
		uint16_t *p;
		p = dac_buffin;
		dac_buffin = dac_buffout;
		dac_buffout = p;
		dac_dataprocflag = 1;
		dac_datawaitfullflag = 0;
		return Chip_GPDMA_Transfer(LPC_GPDMA, dmaChannelNum,
					  (uint32_t) dac_buffin,
					  GPDMA_CONN_DAC,
					  GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
					  160*4);
	}
	else		//the other buffer is in process, we have to wait until interrupt event
	{
		if(dac_datawaitfullflag == 0){
			dac_datawaitfullflag = 1;
		}
		else{
			return ERROR;
		}
	}

	return SUCCESS;
}




void DAC_DMA_stop(){
	/* Disable interrupts, release DMA channel */
	Chip_GPDMA_Stop(LPC_GPDMA, dmaChannelNum);
	NVIC_DisableIRQ(DMA_IRQn);
}



/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	DMA interrupt handler sub-routine
 * @return	Nothing
 */
void DMA_IRQHandler(void)
{
	if (Chip_GPDMA_Interrupt(LPC_GPDMA, dmaChannelNum) == SUCCESS) {
		if(dac_datawaitfullflag == 1){
			//we have
			NVIC_EnableIRQ(DMA_IRQn);
			dmaChannelNum = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_DAC);
			uint16_t *p;
			p = dac_buffin;
			dac_buffin = dac_buffout;
			dac_buffout = p;
			dac_dataprocflag = 1;
			dac_datawaitfullflag = 0;
			Chip_GPDMA_Transfer(LPC_GPDMA, dmaChannelNum,
						  (uint32_t) dac_buffin,
						  GPDMA_CONN_DAC,
						  GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA,
						  160*4);
		}
		else{
			dac_dataprocflag = 0;
		}
	}
	else {
		DAC_DMA_stop();
	}
}



void DAC_init(uint32_t rate)
{
	dac_buffout = dac_buff1;
	dac_buffin = dac_buff2;
	dac_datawaitfullflag = 0;
	uint32_t dacClk;
	Chip_DAC_Init(LPC_DAC);
	Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_DAC, SYSCTL_CLKDIV_1);
	Chip_DAC_SetDMATimeOut(LPC_DAC, DAC_TIMEOUT);
	dacClk = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_DAC);
	/* Enable count and DMA support */
	Chip_DAC_ConfigDAConverterControl(LPC_DAC, DAC_CNT_ENA | DAC_DMA_ENA);
	Chip_DAC_SetDMATimeOut(LPC_DAC,dacClk/rate);
	/* Initialize GPDMA controller */
	Chip_GPDMA_Init(LPC_GPDMA);
	/* Setup GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);
	NVIC_SetPriority(DMA_IRQn, ((0x01 << 3) | 0x01));
}
