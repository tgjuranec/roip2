/*
 * speex_udp.c
 *
 *  Created on: Aug 27, 2016
 *      Author: tgjuranec
 */
#include <speex/speex.h>
#include "lwip/udp.h"
#include "lwip/dns.h"
#include <string.h>
#include <assert.h>
#include <board.h>

#define BLOCK_LENGTH 160U
void *enc_state;
SpeexBits enc_bits;



/*
 * DATA UDP STATE
 */
typedef enum spudp_state_t{
	DISCONNECTED,
	SERVER1,
	SERVER2
} spudp_state;

struct spudp_data{
	struct udp_pcb *p;
	spudp_state st;
	ip_addr_t server1;
	ip_addr_t server2;
};

struct spudp_data dt;

/*
 * CONTROL UDP STATE
 */
struct udp_pcb *pctl;


/*
 * TIMER VARIABLES
 */
uint32_t tm_ctlpacket_sent;
uint32_t tm_ctlpacket_received;


void handleAudioInput(void *arg, struct udp_pcb *pcb, struct pbuf *pb,\
		ip_addr_t *addr, u16_t port)
{
  pbuf_free(pb);
}

void handleControlInput(void *arg, struct udp_pcb *pcb, struct pbuf *pb,\
		ip_addr_t *addr, u16_t port){
	if(dt.st == SERVER1){
		if(ip_addr_cmp(addr,&dt.server1)){
			tm_ctlpacket_received = sys_now();
		}
	}
	else if (dt.st == SERVER2){
		if(ip_addr_cmp(addr,&dt.server1)){
			tm_ctlpacket_received = sys_now();
		}
	}
	else {
		//errror
	}
	pbuf_free(pb);
}


ip_addr_t *get_server1_ip(){
	return &dt.server1;
}

ip_addr_t *get_server2_ip(){
	return &dt.server2;
}

void spudp_init(ip_addr_t *local_ip,ip_addr_t *server1_ip, ip_addr_t *server2_ip){
	uint32_t tmp = 0;
	speex_bits_init(&enc_bits);
	enc_state = speex_encoder_init(&speex_nb_mode);
	tmp = 0;
    speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &tmp);
    tmp = 8;
    speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &tmp);
    tmp = 4;
    speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &tmp);
    tmp = 8000;
    speex_encoder_ctl(enc_state, SPEEX_SET_SAMPLING_RATE, &tmp);
    //tmp =6000;
    //speex_encoder_ctl(enc_state, SPEEX_SET_BITRATE, &tmp);
    //init udp DATA

    if(server1_ip != NULL) dt.server1.addr = server1_ip->addr;
    if(server2_ip != NULL) dt.server2.addr = server2_ip->addr;
    dt.st = SERVER1;
    dt.p = udp_new();
    if(udp_bind(dt.p,local_ip,5198) != ERR_OK){
  	  return;
    }
    udp_recv(dt.p,handleAudioInput,NULL);
    //init udp CONTROL
    pctl = udp_new();
    if(udp_bind(pctl,local_ip,5199) != ERR_OK){
    	return;
    }
    udp_recv(pctl,handleControlInput,NULL);
    //init timers
    tm_ctlpacket_sent = sys_now();
    tm_ctlpacket_received = sys_now();

}

void spudp_deinit(){
	speex_bits_destroy(&enc_bits);
	speex_encoder_destroy(enc_state);
	udp_remove(dt.p);
	dt.p = 0;
	dt.st = DISCONNECTED;
}

uint32_t spudp_send(char *id, char *gid, char *id2, int16_t *samples, uint32_t len){
	char enc[256];
	static uint32_t error;
	uint8_t c = 0;
	uint8_t id_len = strlen(id);
	uint8_t gid_len = strlen(gid);
	uint8_t id2_len = strlen(id2);
	uint8_t all_len = id_len + gid_len + id2_len+3;
	memcpy(&enc[c],&all_len,1);
	c += 1;
	memcpy(&enc[c],id,id_len);
	c += id_len;
	enc[c] = ',';
	c++;
	memcpy(&enc[c],gid,gid_len);
	c += gid_len;
	enc[c] = ',';
	c++;
	memcpy(&enc[c],id2,id2_len);
	c += id2_len;
	//speex encoding

	if(dt.p == 0){
		return 0;
	}

	uint32_t i, nbytes;
	for(i = 0; i< len; i+=BLOCK_LENGTH){
		if(speex_encode_int(enc_state,(spx_int16_t *) &samples[i],&enc_bits)== 0){
			error++;
		}
	}
	speex_bits_insert_terminator(&enc_bits);
	size_t nsize = speex_bits_nbytes(&enc_bits);
	if(nsize < sizeof(enc)){
		nbytes = speex_bits_write(&enc_bits,&enc[c],256);
	}
	speex_bits_reset(&enc_bits);
	udp_connect(dt.p,&dt.server1,5198);
	struct pbuf *pb;
	pb = pbuf_alloc(PBUF_TRANSPORT,nbytes+all_len,PBUF_RAM);
	memcpy(pb->payload,enc,all_len+nbytes);
	if(udp_send(dt.p,pb) != ERR_OK){
		pbuf_free(pb);
		return 0;
	}
	pbuf_free(pb);
	udp_disconnect(dt.p);
	return len;
}

/*
 *
 */

uint32_t spudp_string_send(uint8_t *buf, uint32_t len){
	assert(buf != NULL);
	assert(len > 0);
	assert(len < 256);
	if(dt.st == SERVER1){
		udp_connect(pctl,&dt.server1,5199);
	}
	else if (dt.st == SERVER2){
		udp_connect(pctl,&dt.server2,5199);
	}
	else {
		//errror or we are in DISCONNECTED state
		return 0;
	}
	struct pbuf *pb;
	pb = pbuf_alloc(PBUF_TRANSPORT,len,PBUF_RAM);
	memcpy(pb->payload,buf, len);
	if(udp_send(pctl,pb) != ERR_OK){
		pbuf_free(pb);
		udp_disconnect(pctl);
		return 0;
	}
	pbuf_free(pb);
	udp_disconnect(pctl);
	return len;
}

/*
 * Function for periodic sending of the control packets
 */


void spudp_control_send(){
	uint32_t payload[] = {1,2,3,4,5,6,7,8};
	/*
	 * send syncro packet
	 * if time since last sending is greater than 5s
	 * and last response from server is received
	 */

	if((sys_now() - tm_ctlpacket_sent) > 5000){
		if(tm_ctlpacket_sent <= tm_ctlpacket_received){
			/*
			 * we received last response from server
			 * we are sending another chalenge
			 */
			if(spudp_string_send((uint8_t *)payload,32) < 0){

			}
			tm_ctlpacket_sent = sys_now();
		}
		else{
			/*
			 * we haven't received response from server so far
			 * we are changing server
			 * we are sending another chalenge
			 */
			if(dt.st == SERVER1){
				dt.st = SERVER2;
			}
			else if (dt.st == SERVER2){
				dt.st = SERVER1;
			}
			else {
				//errror
			}
			if(spudp_string_send((uint8_t *)payload,32) < 0){

			}
			tm_ctlpacket_sent = sys_now();
		}
	}

}


