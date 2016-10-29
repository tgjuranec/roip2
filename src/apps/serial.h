/*
 * serial.h
 *
 *  Created on: Sep 17, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_SERIAL_H_
#define APPS_SERIAL_H_

void serial_init();
int serial_send(const uint8_t *buf, int len);
int serial_nitems_RB();
int serial_read(uint8_t *buf, int len);


#endif /* APPS_SERIAL_H_ */
