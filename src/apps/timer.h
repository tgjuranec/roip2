/*
 * timer.h
 *
 *  Created on: Jan 16, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_TIMER_H_
#define APPS_TIMER_H_

typedef void (*timer_callback)(void *arg);
extern uint8_t interrupt_occured[4];
/*
 * select the timer and frequency of interrupts
 * param ntimer - select timer (0 to 3) otherwise function ret zero
 * param t_freq - frequency of interrupts
 * param tc - callback function (can be null if you don't need it
 * param arg - argument that will be passed as argument of callback
 */
void timer_init(uint8_t ntimer, uint32_t t_freq, timer_callback tc, void *arg);

void inline timer_enable(uint8_t ntimer);

void inline timer_disable(uint8_t ntimer);

#endif /* APPS_TIMER_H_ */
