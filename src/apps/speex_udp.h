/*
 * speex_udp.h
 *
 *  Created on: Aug 27, 2016
 *      Author: tgjuranec
 */

#ifndef APPS_SPEEX_UDP_H_
#define APPS_SPEEX_UDP_H_

#include "ipv4/lwip/ip_addr.h"

void spudp_init(ip_addr_t *local_ip,ip_addr_t *server1_ip, ip_addr_t *server2_ip);
void spudp_deinit();
uint32_t spudp_send(char *id, char *gid, char *id2, int16_t *samples, uint32_t len);
ip_addr_t *get_server1_ip();
ip_addr_t *get_server2_ip();
void spudp_control_send();
uint32_t spudp_string_send(uint8_t *buf, uint32_t len);
#endif /* APPS_SPEEX_UDP_H_ */
