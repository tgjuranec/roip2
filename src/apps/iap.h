/*
 * iap.h
 *
 *  Created on: Oct 13, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_IAP_H_
#define APPS_IAP_H_

#include <ipv4/lwip/ip_addr.h>

struct flash_data{
	ip_addr_t ip_local;
	ip_addr_t ip_gw;
	ip_addr_t ip_nm;
	ip_addr_t ser1;
	ip_addr_t ser2;
	uint8_t mac_address[6];
	char password[256-20];
};


extern uint8_t flash_buff[];




void get_flash();
int write_flash(void);

#endif /* APPS_IAP_H_ */
