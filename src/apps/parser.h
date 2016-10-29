/*
 * parser.h
 *
 *  Created on: Oct 3, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_PARSER_H_
#define APPS_PARSER_H_

enum serial_state{
	NOINIT,
	CMD1_SENT,
	CMD2_SENT,
	CMD3_SENT,
	CMD4_SENT,
	CMD5_SENT,
	ACTIVE_WAIT,
	ACTIVE_INCOMING_CALL,
	ACTIVE_CALL_ESTABLISHED,
	ACTIVE_PTT,
	ERROR_STATE
};
extern enum serial_state ser;
extern char ssi[32], ssi2[32],gssi[32];
void radio_comm_init();
void radio_comm();

#endif /* APPS_PARSER_H_ */
