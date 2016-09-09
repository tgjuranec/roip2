/*
 * httpd.c
 *
 *  Created on: Sep 7, 2016
 *      Author: tgjuranec
 */


/* A simple HTTP/1.0 server direcly interfacing the stack. */
#include <lwip/tcp.h>
#include <string.h>

char ip_local[] = "192.168.1.201";
char ip_nm[] = "255.255.255.0";
char ip_gateway[] = "192.168.1.1";
char ip_server[] = "192.168.1.198";

/* This is the data for the actual web page. */
static char pg_header[] =
"HTTP/1.0 200 OK\r\n\
Content-type: text/html\r\n\
\r\n\
<html> \
<head><title>A test page</title></head> \
<body> \
<h2> This is a small test page. </h2>";

static char pg_form0[] = "<form action=\"demo_form.asp\"> \
		Local IP address: <input type=\"text\" name=\"ip_local\" value=\"";
static char pg_form1[] = "\"><br>\
		Gateway IP address: <input type=\"text\" name=\"ip_gateway\" value=\"";
static char pg_form2[] = "\"><br>\
		<input type=\"submit\" value=\"APPLY\">\
		</form>";

static char pg_tail[] ="</body> \
</html>";

const static char request[] = "GET /";

char outdata[512];


/* This is the callback function that is called
when a TCP segment has arrived in the connection. */
static err_t
http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char *rq;
	/* If we got a NULL pbuf in p, the remote end has closed
	the connection. */
	if(p != NULL) {

		tcp_recved(pcb,p->tot_len);
		/* The payload pointer in the pbuf contains the data
		in the TCP segment. */
		rq = p->payload;
		/* Check
		 if
		 the request was
		 an HTTP "GET /\r\n". */
		if(strncmp(rq,request,strlen(request)) == 0) {
			/* Send the web page to the remote host. A zero
			in the last argument means that the data should
			not be copied into internal buffers. */
			outdata[0] = 0;
			strcat(outdata,pg_header);
			strcat(outdata,pg_form0);
			strcat(outdata,ip_local);
			strcat(outdata,pg_form1);
			strcat(outdata,ip_gateway);
			strcat(outdata,pg_form2);
			strcat(outdata,pg_tail);
			tcp_write(pcb, outdata, strlen(outdata), 0);
		}
		/* Free the pbuf. */
		pbuf_free(p);
	}
	/* Close the connection. */
	tcp_close(pcb);
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
