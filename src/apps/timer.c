/*
 * timer.c
 *
 *  Created on: Jan 16, 2016
 *      Author: tgjuranec
 */

#include <chip.h>
#include "timer.h"

timer_callback fn[4];
void *args[4];
uint8_t interrupt_occured[4];

void timer_init(uint8_t ntimer, uint32_t t_freq, timer_callback tc, void *arg){
	LPC_TIMER_T *ptmr;
	IRQn_Type IRQn;
	switch(ntimer){
	case 0:
		ptmr = LPC_TIMER0;
		IRQn = TIMER0_IRQn;
		if(tc != 0){
			fn[0] = tc;
			args[0] = arg;
		}
		break;
	case 1:
		ptmr = LPC_TIMER1;
		IRQn = TIMER1_IRQn;
		if(tc != 0){
			fn[1] = tc;
			args[0] = arg;
		}
		break;
	case 2:
		ptmr = LPC_TIMER2;
		IRQn = TIMER2_IRQn;
		if(tc != 0){
			fn[2] = tc;
			args[0] = arg;
		}
		break;
	case 3:
		ptmr = LPC_TIMER3;
		IRQn = TIMER3_IRQn;
		if(tc != 0){
			fn[3] = tc;
			args[0] = arg;
		}
		break;
	default:
		return;
	}

	/* Enable timer 1 clock */
	Chip_TIMER_Init(ptmr);

	/* Timer rate is system clock rate */
	uint32_t timerFreq = Chip_Clock_GetSystemClockRate();

	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(ptmr);
	Chip_TIMER_MatchEnableInt(ptmr, 1);
	Chip_TIMER_SetMatch(ptmr, 1, (timerFreq / t_freq));
	Chip_TIMER_ResetOnMatchEnable(ptmr, 1);
	Chip_TIMER_Disable(ptmr);
	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(IRQn);
	NVIC_EnableIRQ(IRQn);
}

void inline timer_enable(uint8_t ntimer){
	LPC_TIMER_T *ptmr;
	switch(ntimer){
	case 0:
		ptmr = LPC_TIMER0;
		break;
	case 1:
		ptmr = LPC_TIMER1;
		break;
	case 2:
		ptmr = LPC_TIMER2;
		break;
	case 3:
		ptmr = LPC_TIMER3;
		break;
	default:
		return;
	}
	Chip_TIMER_Enable(ptmr);
}

void inline timer_disable(uint8_t ntimer){
	LPC_TIMER_T *ptmr;
	switch(ntimer){
	case 0:
		ptmr = LPC_TIMER0;
		break;
	case 1:
		ptmr = LPC_TIMER1;
		break;
	case 2:
		ptmr = LPC_TIMER2;
		break;
	case 3:
		ptmr = LPC_TIMER3;
		break;
	default:
		return;
	}
	Chip_TIMER_Disable(ptmr);
}


void TIMER0_IRQHandler(void)
{
	if (Chip_TIMER_MatchPending(LPC_TIMER0, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER0, 1);
		if(fn[0] != 0){
			fn[0](args[0]);
		}
		interrupt_occured[0]++;
	}
}


void TIMER1_IRQHandler(void)
{
	if (Chip_TIMER_MatchPending(LPC_TIMER1, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER1, 1);
		if(fn[1] != 0){
			fn[1](args[1]);
		}
	}
}


void TIMER2_IRQHandler(void)
{
	if (Chip_TIMER_MatchPending(LPC_TIMER2, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER2, 1);
		if(fn[2] != 0){
			fn[2](args[2]);
		}
	}
}


void TIMER3_IRQHandler(void)
{
	if (Chip_TIMER_MatchPending(LPC_TIMER3, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER3, 1);
		if(fn[3] != 0){
			fn[3](args[3]);
		}
	}
}
