/*
 * httpd.c
 *
 *  Created on: Sep 7, 2016
 *      Author: tgjuranec
 */

#include <board.h>
/* A simple HTTP/1.0 server direcly interfacing the stack. */
#include <lwip/tcp.h>
#include <lwip/netif.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <speex_udp.h>
#include "iap.h"
#include "roip.h"

//macro for finding digit in  string
#define FIND_EQUALS(p,end) do{p++;}while(((*p) != '=') && ((p) < (end)))
#define FIND_QUESTMARK(p,end) do{p++;}while(((*p) != '?') && ((p) < (end)))
#define FIND_SPACE(p,end) do{p++;}while(((*p) != ' ') && ((p) < (end)))
#define FIND_AMPERSAND(p,end) do{p++;}while(((*p) != '&') && ((p) < (end)))

uint32_t sysreset_req;

/* This is the data for the actual web page. */
static char pg_header[] =
"HTTP/1.0 200 OK\r\n\
Content-type: text/html\r\n\
\r\n\
<html> \
<head><title>Network</title></head> \
<body> \
<h2><b> The current network configuration </b></h2>";

const static char pg_form_pass[] = "<form action=\"pass_sent.cgi\"> \
		Enter password: <input type=\"text\" name=\"password\" maxlength=\"15\" size=\"15\" value=\"";


const static char pg_form0[] = "<form action=\"ip_set.cgi\"> \
		Local IP address: <input type=\"text\" name=\"ip_local1\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form1[] = "\"><input type=\"text\" name=\"ip_local2\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form2[] = "\"><input type=\"text\" name=\"ip_local3\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form3[] = "\"><input type=\"text\" name=\"ip_local4\" maxlength=\"3\" size=\"3\" value=\"";

const static char pg_form4[] = "\"><br>\
		Gateway IP address: <input type=\"text\" name=\"ip_gateway1\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form5[] = "\"><input type=\"text\" name=\"ip_gateway2\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form6[] = "\"><input type=\"text\" name=\"ip_gateway3\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form7[] = "\"><input type=\"text\" name=\"ip_gateway4\" maxlength=\"3\" size=\"3\" value=\"";

const static char pg_form8[] = "\"><br>\
		Network mask: <input type=\"text\" name=\"ip_nm1\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form9[] = "\"><input type=\"text\" name=\"ip_nm2\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form10[] = "\"><input type=\"text\" name=\"ip_nm3\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form11[] = "\"><input type=\"text\" name=\"ip_nm4\" maxlength=\"3\" size=\"3\" value=\"";

const static char pg_form12[] = "\"><br>\
		Server 1: <input type=\"text\" name=\"ip_server11\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form13[] = "\"><input type=\"text\" name=\"ip_server12\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form14[] = "\"><input type=\"text\" name=\"ip_server13\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form15[] = "\"><input type=\"text\" name=\"ip_server14\" maxlength=\"3\" size=\"3\" value=\"";

const static char pg_form16[] = "\"><br>\
		Server 2: <input type=\"text\" name=\"ip_server21\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form17[] = "\"><input type=\"text\" name=\"ip_server22\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form18[] = "\"><input type=\"text\" name=\"ip_server23\" maxlength=\"3\" size=\"3\" value=\"";
const static char pg_form19[] = "\"><input type=\"text\" name=\"ip_server24\" maxlength=\"3\" size=\"3\" value=\"";

const static char pg_form20[] = "\"><br> <br>\
		Change password: <input type=\"text\" name=\"new_password\" maxlength=\"15\" size=\"15\" value=\"";
const static char pg_form21[] = "\"><br>\
		Change password: <input type=\"text\" name=\"confirm_password\" maxlength=\"15\" size=\"15\" value=\"";

const static char pg_changed_configuration[] = "<p>Please wait, chip is going to reset.</p>";
const static char pg_wrong_password[] = "<p>You've just entered the wrong password!</p>";
const static char pg_change_password_unsuccessful[] = "<p>There was an error changing password!</p>";

const static char pg_form_tail[] = "\"><br>\
		<input type=\"submit\" value=\"APPLY\">\
		</form>";

const static char pg_tail[] ="</body> \
</html>";


const static char *str_ip_local1 = "ip_local1";
const static char *str_ip_local2 = "ip_local2";
const static char *str_ip_local3 = "ip_local3";
const static char *str_ip_local4 = "ip_local4";
const static char *str_ip_gw1 = "ip_gateway1";
const static char *str_ip_gw2 = "ip_gateway2";
const static char *str_ip_gw3 = "ip_gateway3";
const static char *str_ip_gw4 = "ip_gateway4";
const static char *str_ip_nm1 = "ip_nm1";
const static char *str_ip_nm2 = "ip_nm2";
const static char *str_ip_nm3 = "ip_nm3";
const static char *str_ip_nm4 = "ip_nm4";
const static char *str_ip_ser11 = "ip_server11";
const static char *str_ip_ser12 = "ip_server12";
const static char *str_ip_ser13 = "ip_server13";
const static char *str_ip_ser14 = "ip_server14";
const static char *str_ip_ser21 = "ip_server21";
const static char *str_ip_ser22 = "ip_server22";
const static char *str_ip_ser23 = "ip_server23";
const static char *str_ip_ser24 = "ip_server24";
const static char *str_ip_newpass = "new_password";
const static char *str_ip_confirm_pass = "confirm_password";



char ip_local1[4];
char ip_local2[4];
char ip_local3[4];
char ip_local4[4];

char ip_gateway1[4];
char ip_gateway2[4];
char ip_gateway3[4];
char ip_gateway4[4];

char ip_nm1[4];
char ip_nm2[4];
char ip_nm3[4];
char ip_nm4[4];

char ip_server11[4];
char ip_server12[4];
char ip_server13[4];
char ip_server14[4];

char ip_server21[4];
char ip_server22[4];
char ip_server23[4];
char ip_server24[4];

//requests
const static char request[] = "GET /";
const static char ip_set[] = "ip_set.cgi";
const static char pass_sent[] = "pass_sent.cgi";

char outdata[2048];

static err_t
http_sent(void *arg, struct tcp_pcb *tpcb,u16_t len){
	return ERR_OK;
}



/* This is the callback function that is called
when a TCP segment has arrived in the connection. */
static err_t
http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char *rq;
	struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*)arg;
	/* If we got a NULL pbuf in p, the remote end has closed
	the connection. */
	if(p != NULL) {
		char *packet_end = ((char *)p->payload) + p->tot_len;
		char *end_of_string;
		tcp_recved(pcb,p->tot_len);
		/* The payload pointer in the pbuf contains the data
		in the TCP segment. */
		rq = p->payload;

		/* Check HTTP "GET /". */
		if(strncmp(rq,request,strlen(request)) == 0) {
			/* Check HTTP "GET /\r\n". */
			/* SEND PASSWORD REQUEST*/
			if(rq[5] == ' '){
				outdata[0] = 0;
				strcat(outdata,pg_header);
				strcat(outdata,pg_form_pass);
				strcat(outdata,pg_form_tail);
				strcat(outdata,pg_tail);
				uint8_t error = 0;
				if(tcp_write(pcb, outdata, strlen(outdata), TCP_WRITE_FLAG_COPY) != ERR_OK){
					//ERROR
					error++;
				}
				if(tcp_output(pcb) != ERR_OK){
					//ERROR
					error++;
				}
				/* Close the connection. */
				if(tcp_close(pcb) != ERR_OK){
					error++;
				}
			}
			else{
				//GET IP ADDRESSES
				//get from netif
				ip_addr_t ipl, ipgw, ipnm;
				ipl.addr = lpc_netif.ip_addr.addr;
				ipgw.addr = lpc_netif.gw.addr;
				ipnm.addr = lpc_netif.netmask.addr;
				//get from udp system
				ip_addr_t server1_ip,server2_ip;
				server1_ip.addr = get_server1_ip()->addr;
				server2_ip.addr = get_server2_ip()->addr;
				sprintf(ip_local1,"%u",ip4_addr1(&ipl));
				sprintf(ip_local2,"%u",ip4_addr2(&ipl));
				sprintf(ip_local3,"%u",ip4_addr3(&ipl));
				sprintf(ip_local4,"%u",ip4_addr4(&ipl));

				sprintf(ip_gateway1,"%u",ip4_addr1(&ipgw));
				sprintf(ip_gateway2,"%u",ip4_addr2(&ipgw));
				sprintf(ip_gateway3,"%u",ip4_addr3(&ipgw));
				sprintf(ip_gateway4,"%u",ip4_addr4(&ipgw));

				sprintf(ip_nm1,"%u",ip4_addr1(&ipnm));
				sprintf(ip_nm2,"%u",ip4_addr2(&ipnm));
				sprintf(ip_nm3,"%u",ip4_addr3(&ipnm));
				sprintf(ip_nm4,"%u",ip4_addr4(&ipnm));

				sprintf(ip_server11,"%u",ip4_addr1(&server1_ip));
				sprintf(ip_server12,"%u",ip4_addr2(&server1_ip));
				sprintf(ip_server13,"%u",ip4_addr3(&server1_ip));
				sprintf(ip_server14,"%u",ip4_addr4(&server1_ip));

				sprintf(ip_server21,"%u",ip4_addr1(&server2_ip));
				sprintf(ip_server22,"%u",ip4_addr2(&server2_ip));
				sprintf(ip_server23,"%u",ip4_addr3(&server2_ip));
				sprintf(ip_server24,"%u",ip4_addr4(&server2_ip));


				outdata[0] = 0;
				strcat(outdata,pg_header);
				strcat(outdata,pg_form0);
				strcat(outdata,ip_local1);
				strcat(outdata,pg_form1);
				strcat(outdata,ip_local2);
				strcat(outdata,pg_form2);
				strcat(outdata,ip_local3);
				strcat(outdata,pg_form3);
				strcat(outdata,ip_local4);
				strcat(outdata,pg_form4);
				strcat(outdata,ip_gateway1);
				strcat(outdata,pg_form5);
				strcat(outdata,ip_gateway2);
				strcat(outdata,pg_form6);
				strcat(outdata,ip_gateway3);
				strcat(outdata,pg_form7);
				strcat(outdata,ip_gateway4);
				strcat(outdata,pg_form8);
				strcat(outdata,ip_nm1);
				strcat(outdata,pg_form9);
				strcat(outdata,ip_nm2);
				strcat(outdata,pg_form10);
				strcat(outdata,ip_nm3);
				strcat(outdata,pg_form11);
				strcat(outdata,ip_nm4);
				strcat(outdata,pg_form12);
				strcat(outdata,ip_server11);
				strcat(outdata,pg_form13);
				strcat(outdata,ip_server12);
				strcat(outdata,pg_form14);
				strcat(outdata,ip_server13);
				strcat(outdata,pg_form15);
				strcat(outdata,ip_server14);
				strcat(outdata,pg_form16);
				strcat(outdata,ip_server21);
				strcat(outdata,pg_form17);
				strcat(outdata,ip_server22);
				strcat(outdata,pg_form18);
				strcat(outdata,ip_server23);
				strcat(outdata,pg_form19);
				strcat(outdata,ip_server24);
				strcat(outdata,pg_form20);
				strcat(outdata,pg_form21);
				strcat(outdata,pg_form_tail);

				//CHECK "GET /pass_sent.cgi - CHECKING PASSWORD, SEND EXISTING DATA
				if(strncmp(&rq[5],pass_sent,strlen(pass_sent)) == 0){
					/*checking password*/
					struct flash_data *fd;
					get_flash();
					char *pass;
					fd = (struct flash_data *) flash_buff;
					pass = fd->password;
					char *new_setup = &rq[18];
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;


					if(strncmp(pass,new_setup,strlen(pass)) == 0){
						/*password OK*/
						strcat(outdata,pg_tail);
					}
					else{
						/*password wrong*/
						outdata[0] = 0;
						strcat(outdata,pg_header);
						strcat(outdata,pg_wrong_password);
						strcat(outdata,pg_tail);
					}

					uint8_t error = 0;
					if(tcp_write(pcb, outdata, strlen(outdata), TCP_WRITE_FLAG_COPY) != ERR_OK){
						//ERROR
						error++;
					}
					if(tcp_output(pcb) != ERR_OK){
						//ERROR
						error++;
					}
					/* Close the connection. */
					if(tcp_close(pcb) != ERR_OK){
						error++;
					}
				}

				//'GET /ip_set.cgi' CHANGING network configuration, and Password
				else if (strncmp(&rq[5],ip_set,strlen(ip_set)) == 0){
					strcat(outdata,pg_changed_configuration);
					strcat(outdata,pg_tail);
					uint8_t error = 0;
					if(tcp_write(pcb, outdata, strlen(outdata), TCP_WRITE_FLAG_COPY) != ERR_OK){
						//ERROR
						error++;
					}
					if(tcp_output(pcb) != ERR_OK){
						//ERROR
						error++;
					}
					/* Close the connection. */
					if(tcp_close(pcb) != ERR_OK){
						error++;
					}
					//First set network interface
					char *new_setup = &rq[26]; //new setup starts at the 17th character of the request (rq[16])
					char **delimiter = &new_setup;
					//spudp_deinit();
					uint8_t ip1,ip2,ip3,ip4;
					ip_addr_t ip_tmplocal, ip_tmpgw, ip_tmpnm, ip_tmpser1,ip_tmpser2;
					ip1 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip2 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip3 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip4 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					IP4_ADDR(&ip_tmplocal,ip1,ip2,ip3,ip4);
					//netif_set_ipaddr(&lpc_netif,&ip_tmplocal);
					ip1 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip2 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip3 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip4 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					IP4_ADDR(&ip_tmpgw,ip1,ip2,ip3,ip4);
					//netif_set_gw(&lpc_netif,&ip_tmpgw);
					ip1 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip2 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip3 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip4 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					IP4_ADDR(&ip_tmpnm,ip1,ip2,ip3,ip4);
					//netif_set_netmask(&lpc_netif,&ip_tmpnm);


					//set servers
					ip1 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip2 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip3 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip4 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					IP4_ADDR(&ip_tmpser1,ip1,ip2,ip3,ip4);

					ip1 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip2 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip3 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);
					FIND_EQUALS(new_setup,packet_end);
					new_setup++;
					ip4 = (uint8_t) (strtoul(new_setup,delimiter,10) & 0xFF);
					new_setup = (*delimiter);

					IP4_ADDR(&ip_tmpser2,ip1,ip2,ip3,ip4);
					//spudp_init(&lpc_netif.ip_addr,&ip_tmpser1,&ip_tmpser2);
					//check new password
					char new_password[16],confirm_password[16];

					/*
					 * find start end end of password string in the payload
					 * start - new_setup
					 * end - end_of_string
					 * copy into new_password string
					 */
					FIND_EQUALS(new_setup,packet_end);
					end_of_string = new_setup;
					FIND_AMPERSAND(end_of_string,packet_end);
					new_setup++;
					memcpy(new_password,new_setup,end_of_string-new_setup);
					new_password[end_of_string-new_setup] = 0;
					FIND_EQUALS(new_setup,packet_end);
					end_of_string = new_setup;
					FIND_SPACE(end_of_string,packet_end);
					new_setup++;
					memcpy(confirm_password,new_setup,end_of_string-new_setup);
					confirm_password[end_of_string-new_setup] = 0;


					struct flash_data *fd = (struct flash_data *) flash_buff;
					ip_addr_copy(fd->ip_local,ip_tmplocal);
					ip_addr_copy(fd->ip_gw, ip_tmpgw);
					ip_addr_copy(fd->ip_nm,ip_tmpnm);
					ip_addr_copy(fd->ser1,ip_tmpser1);
					ip_addr_copy(fd->ser2, ip_tmpser2);
					memcpy(fd->password,new_password,strlen(new_password));
					fd->password[strlen(new_password)] = 0;
					write_flash();
					sysreset_req = sys_now();

				}
				else{

				}
			}

		}
		/* Free the pbuf. */
		pbuf_free(p);
	}

	return ERR_OK;
}


/* This is the callback function that is called when
a connection has been accepted. */
static err_t
http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*)arg;
	tcp_accepted(lpcb);
	/* Set up the function http_recv() to be called when data
	arrives. */
	tcp_recv(pcb, http_recv);
	tcp_arg(pcb,lpcb);
	tcp_sent(pcb,http_sent);
	return ERR_OK;
}

/* The initialization function. */
void
http_init(void)
{
	struct tcp_pcb *pcb;
	/* Create a new TCP PCB. */
	pcb = tcp_new();
	/* Bind the PCB to TCP port 80. */
	tcp_bind(pcb, IP_ADDR_ANY, 80);
	/* Change TCP state to LISTEN. */
	pcb = tcp_listen(pcb);
	tcp_arg(pcb,pcb);
	/* Set up http_accet() function to be called
	when a new connection arrives. */
	tcp_accept(pcb, http_accept);
}
