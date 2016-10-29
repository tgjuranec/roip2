/*
 * parser.c
 *
 *  Created on: Oct 3, 2016
 *      Author: tgjuranec
 */

#include <string.h>
#include <board.h>
#include "parser.h"
#include "serial.h"
#include "adc.h"
#include "speex_udp.h"
#include <arch/lpc_arch.h>

const static char str1[]= "AT+CTSDS=12,0,0,0,0\r\n";
const static char str2[] =  "AT+CTSP=1,3,130\r\n";
const static char str3[] = "AT+CTSP=1,1,11\r\n";
const static char str4[] = "AT+CTSP=2,0,0\r\n";
const static char str5[] = "AT+CTSP=2,2,20\r\n";



//Incoming group call GSSI:300 , Caller SSI 255 :
const static char get_ssi[]= "+CTICN:0,0,0,0,255,1,1,0,1,1,1,216216300000300,3";
//Call established
const static char call_est[]= "+CTCC:0,1,1,0,0,1,1,3";
//PTT is at SSI 255:
const static char ptt_pressed[]= "+CTXG:0,3,0,0,0,255";
//PPT is released:
const static char ptt_released[]= "+CDTXC:0,0";
//Call end:
const static char call_end[]= "+CTCR:0,13";

//newline
const static char crln[] = "\r\n";

enum serial_state ser;
char ssi[32], ssi2[32],gssi[32];

/*
 * function for parsing values from string which are delimited with commas
 * buf - parsed string
 * place - position where we expect wanted value (start from 1)
 * len - length of the output value
 */

static char *get_value(char *buf, uint32_t len, uint8_t place, uint8_t *out_len){
	char *start = 0;
	uint32_t c = 0;
	uint8_t count_place = 1;
	while(c < len){
		start = &buf[c];
		uint8_t seg_len = 0;
		while(buf[c] >= 0x30 && buf[c] <= 0x39){
			seg_len++;
			c++;
		}
		/* we found result*/
		if(count_place == place){
			*out_len = seg_len;
			return start;
		}
		/*we're proceeding*/
		else{
			count_place++;
			c ++;
		}
	}

	return start;
}

void radio_comm(){
	uint32_t c = 0;
	uint8_t len;
	char *s;
	static uint32_t nbytes = 0;
	static char buf[256];
	nbytes += serial_read((uint8_t *)&buf[nbytes],256-nbytes);
	if(nbytes > 0 && buf[nbytes-1] == '\n'){
		spudp_string_send((uint8_t *)buf,nbytes);	//send onto the server
		while(c < nbytes){
			if(buf[c] == '\r' || buf[c] == '\n'){
				c++;
			}
			else if(strncmp(&buf[c],get_ssi,7) == 0){
				c += 8;
				s = get_value(&buf[c],nbytes-c,5,&len);
				memcpy(ssi,s,len);
				ssi[len] = 0;
				s = get_value(&buf[c],nbytes-c,12,&len);
				memcpy(gssi,s,len);
				gssi[len] = 0;
				ser = ACTIVE_INCOMING_CALL;
			}
			else if(strncmp(&buf[c],call_est,6) == 0){
				c += 7;
				ser = ACTIVE_CALL_ESTABLISHED;
			}
			else if(strncmp(&buf[c],ptt_pressed,6) == 0){
				c += 7;
				s = get_value(&buf[c],nbytes-c,6,&len);
				memcpy(ssi2,s,len);
				ssi2[len] = 0;
				ser = ACTIVE_PTT;
				ADC_start(ADC_CH2);
			}
			else if(strncmp(&buf[c],ptt_released,7) == 0){
				c += 8;
				memset(ssi2,0,32);
				ser = ACTIVE_CALL_ESTABLISHED;
				ADC_stop(ADC_CH2);

			}
			else if(strncmp(&buf[c],call_end,6) == 0){
				c += 7;
				memset(ssi,0,32);
				memset(gssi,0,32);
				ser = ACTIVE_WAIT;
			}
			else{
				c++;
				//do nothing
			}
		}
		nbytes = 0;
	}
}

void radio_comm_init(){
	static uint32_t nbytes = 0;
	char serial_buffer[256];
	memcpy(serial_buffer,str1,strlen(str1));
	msDelay(5000);
	while(serial_send((uint8_t *)serial_buffer,strlen(str1)) == 0);
	ser = CMD1_SENT;
	while (ser < ACTIVE_WAIT){
		nbytes += serial_read((uint8_t *)&serial_buffer[nbytes],256-nbytes);
		if(nbytes > 0){
			/*there are data*/
			if(strncmp(&serial_buffer[nbytes-4],"OK\r\n",4) == 0) {
				/*we received OK*/
				switch(ser){
				case NOINIT:
					/*error*/
					break;
				case CMD1_SENT:
					memcpy(serial_buffer,str2,strlen(str2));
					serial_send((uint8_t *)serial_buffer,strlen(str2));
					ser = CMD2_SENT;
					break;
				case CMD2_SENT:
					memcpy(serial_buffer,str3,strlen(str3));
					serial_send((uint8_t *)serial_buffer,strlen(str3));
					ser = CMD3_SENT;
					break;
				case CMD3_SENT:
					memcpy(serial_buffer,str4,strlen(str4));
					serial_send((uint8_t *)serial_buffer,strlen(str4));
					ser = CMD4_SENT;
					break;
				case CMD4_SENT:
					memcpy(serial_buffer,str5,strlen(str5));
					serial_send((uint8_t *)serial_buffer,strlen(str5));
					ser = CMD5_SENT;
					break;
				case CMD5_SENT:
					ser = ACTIVE_WAIT;
					break;
				case ACTIVE_WAIT:
					break;
				case ERROR_STATE:
					break;
				default:
					break;
				}

			}
			else {
				/*some error*/
				/*lets go from the start*/
				memcpy(serial_buffer,str1,strlen(str1));
				serial_send((uint8_t *)serial_buffer,strlen(str1));
				ser = CMD1_SENT;

			}
			nbytes = 0;
		}
	}
}


