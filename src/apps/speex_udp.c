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


#define BLOCK_LENGTH 160U
void *enc_state;
SpeexBits enc_bits;

struct udp_pcb *p;

void handleAudioInput(void *arg, struct udp_pcb *pcb, struct pbuf *pb,\
		ip_addr_t *addr, u16_t port)
{
  pbuf_free(pb);
}


void spudp_init(ip_addr_t *local_ip){
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
    //init udp
    p = udp_new();
    if(udp_bind(p,local_ip,5198) != ERR_OK){
  	  return;
    }
	//udp_connect(p,rip,5198);
    udp_recv(p,handleAudioInput,NULL);
}

void spudp_deinit(){
	speex_bits_destroy(&enc_bits);
	speex_encoder_destroy(enc_state);
	udp_remove(p);
}

uint32_t spudp_send(char *id, int16_t *samples, uint32_t len, ip_addr_t *rip){
	static char enc[256];
	static uint32_t error;


	uint8_t id_len = strlen(id);
	memcpy(&enc[0],&id_len,1);
	memcpy(&enc[1],id,id_len);
	//speex encoding

	uint32_t i, nbytes;
	for(i = 0; i< len; i+=BLOCK_LENGTH){
		if(speex_encode_int(enc_state,(spx_int16_t *) &samples[i],&enc_bits)== 0){
			error++;
		}
	}
	speex_bits_insert_terminator(&enc_bits);
	size_t nsize = speex_bits_nbytes(&enc_bits);
	if(nsize < sizeof(enc)){
		nbytes = speex_bits_write(&enc_bits,&enc[0+id_len+1],256);
	}
	speex_bits_reset(&enc_bits);
	udp_connect(p,rip,5198);
	struct pbuf *pb;
	pb = pbuf_alloc(PBUF_TRANSPORT,nbytes+id_len+1,PBUF_RAM);
	memcpy(pb->payload,enc,nbytes+id_len+1);
	if(udp_send(p,pb) != ERR_OK){
		pbuf_free(pb);
		return 0;
	}
	pbuf_free(pb);
	udp_disconnect(p);
	return len;
}
