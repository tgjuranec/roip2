/*
 * speex_udp.h
 *
 *  Created on: Aug 27, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_SPEEX_UDP_H_
#define APPS_SPEEX_UDP_H_

void spudp_init(ip_addr_t *local_ip);
void spudp_deinit();
uint32_t spudp_send(char *id, int16_t *samples, uint32_t len, ip_addr_t *rip);

#endif /* APPS_SPEEX_UDP_H_ */
