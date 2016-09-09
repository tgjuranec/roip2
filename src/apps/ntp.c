/*
 * ntp.c
 *
 *  Created on: 25.9.2011.
 *      Author: djuka
 */

#include <stdint.h>
#include "ntp.h"
#include "lwip/udp.h"
#include "chip.h"
#include <string.h>
#include <stdio.h>

#include "../lwip/inc/lwip/dns.h"
#include "../lwip/inc/lwip/opt.h"
#include "../lwip/inc/lwip/udp.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

ip_addr_t ntpserver;
//offset of systick milisecond counter
//real miliseconds are systick%1000 + systick_ms_offset
//it can be positive or negative, but always less of 1000!!!
int32_t systick_ms_offset;



static struct ntp_state sntp;
static char *ntp_server_name = "zg1.ntp.carnet.hr";



#define NTP_PORT 			123
#define RTC_DEFAULT_YEAR 	2011
#define CLOCK_SECOND 		1000
#define MILISECOND_TO_FRACTION		4294967

static char str_kod[14][5] = {
		{'A','C','S','T',0},
		{'A','U','T','H',0},
		{'A','U','T','O',0},
		{'B','C','S','T',0},
		{'C','R','Y','P',0},
		{'D','E','N','Y',0},
		{'D','R','O','P',0},
		{'R','S','T','R',0},
		{'I','N','I','T',0},
		{'M','C','S','T',0},
		{'N','K','E','Y',0},
		{'R','A','T','E',0},
		{'R','M','O','T',0},
		{'S','T','E','P',0}
};

uint8_t days_of_month[12] = {
		31,
		28,
		31,
		30,
		31,
		30,
		31,
		31,
		30,
		31,
		30,
		31
};


void set_all_registers(){
	short daycounter, i, yearcounter;
	uint8_t month=0, dom=0, dow=5;			//dow = 5 sathurday at 01.01.2011.
	//correct the dow variable in the next years
	yearcounter = Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR);
	while (yearcounter > RTC_DEFAULT_YEAR){
		yearcounter--;
		if((yearcounter % 4) == 0){
			if((yearcounter % 100) != 0) dow = (dow +1) % 7;
			else {
				if((yearcounter % 400) == 0) dow = (dow +1) % 7;
			}
		}
		dow = (dow + 365) % 7;
	}
	//leap year
	if((Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR) % 4) == 0 ) {
		if((Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR) % 100) == 0) {
			if((Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR) % 400) == 0) days_of_month[1] = 29;
		}
		else days_of_month[1] = 29;
	}

	daycounter = Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_DAYOFYEAR);
	for(i=0; (i<12) && (daycounter > 0);i++){
		if(daycounter <= days_of_month[i] ){			//month is found
			month++;
			dom = daycounter;
			dow = (dow + daycounter) % 7;
			daycounter = 0;
			break;
		}
		else {							//we are searching for new month
			daycounter -= days_of_month[i];
			month++;
			dow = (dow + days_of_month[i]) % 7;
		}
	}
	Chip_RTC_SetTime(LPC_RTC, RTC_TIMETYPE_DAYOFMONTH,dom);
	Chip_RTC_SetTime(LPC_RTC, RTC_TIMETYPE_MONTH,month);
	Chip_RTC_SetTime(LPC_RTC, RTC_TIMETYPE_DAYOFWEEK,dow);
	return;
}


int n_leap_days(date *s, date *e){
	//controlling parameters
	uint16_t cy, nleapdays = 0, leap_year;
	if(s->year > e->year) return -1;
	else if(s->year == e->year){
		if(s->month > e->month) return -1;
		else if(s->month == e->month)
			if(s->day > e->month) return -1;
	}
	//loop for finding leap years
	cy = s->year;
	//correct start year
	if(s->month > 2) cy++;			//There is not leap day anyway
	while(cy <= e->year){
		leap_year = 0;
		if((cy % 4) == 0){
			if((cy % 100) == 0){
				if((cy % 400) == 0){
					leap_year = 1;
				}
			}
			else {
				leap_year = 1;
			}
		}
		if(leap_year == 1){
			nleapdays++;
			//correct last year
			if((cy == e->year) && (e->month < 3)){
				if(!((e->month == 2) && (e->day == 29)))
					nleapdays--;
			}
		}
		cy++;
	}
	return (int) nleapdays;
}


/*function for setting Year, DayOfYear, Hour, Min and Sec register
 * from argument - number of seconds from 1900.
 */
void set_curr_time(uint32_t seconds){
	uint32_t rem= seconds;
	IP_RTC_Enable(LPC_RTC, DISABLE);//stop counter
	uint8_t leap_year;
	uint16_t cy = 1900;
	//we are checking 31st bit of the result finding an overflow -> end loop comparing it w
	//with its old value (stored in seconds variable)
	while(!(((rem >> 31) == 1) && ((seconds >> 31) == 0))){
		seconds = rem;										//for testing end of loop purpose
		leap_year = 0;
		if((cy % 4) == 0){
			if((cy % 100) == 0){
				if((cy % 400) == 0){
					leap_year = 1;
				}
			}
			else {
				leap_year = 1;
			}
		}
		if(leap_year == 0)			rem -= (365*24*60*60);
		else if(leap_year == 1) 	rem -= (366*24*60*60);
		cy++;
		if(rem == 0) break;		//exception for end of loop both 'rem' and 'second's' 31st bit is 0
	}
	if(rem == 0) seconds = 0;	//there is no overflow
	else {						//if there is an overflow -> correct 'rem' to its old value
		rem = seconds;
		cy--;					//correct counteryear variable
	}
	Chip_RTC_SetTime(LPC_RTC,RTC_TIMETYPE_YEAR, cy);
	Chip_RTC_SetTime(LPC_RTC,RTC_TIMETYPE_DAYOFYEAR, ((uint32_t)(rem/(24*60*60))) + 1);
	rem = rem % (24*60*60);
	Chip_RTC_SetTime(LPC_RTC,RTC_TIMETYPE_HOUR, (uint32_t)(rem/(60*60)));
	rem = rem % (60*60);
	Chip_RTC_SetTime(LPC_RTC,RTC_TIMETYPE_MINUTE,(uint32_t)(rem/60));
	rem = rem % (60);
	Chip_RTC_SetTime(LPC_RTC,RTC_TIMETYPE_SECOND,rem);
	set_all_registers();
	IP_RTC_Enable(LPC_RTC, ENABLE);			//start counter
	return;
}


uint32_t get_curr_time(){
	//stop counters
	Chip_RTC_Enable(LPC_RTC, DISABLE);
	uint32_t sum = 0;
	date dcur, d1900;
	//init dcur and d1900
	dcur.year = Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR);
	dcur.month = Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_MONTH);
	dcur.day = Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_DAYOFMONTH);
	d1900.year = 1900;
	d1900.month = 1;
	d1900.day = 1;
	int leapdays;
	if((leapdays = n_leap_days(&d1900,&dcur)) < 0) return 0;
	sum += (Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_YEAR) -1900)*365*24*60*60 + leapdays*24*60*60;
	sum += (Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_DAYOFYEAR) - 1)*24*60*60;
	sum += Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_HOUR) * 60*60;
	sum += Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_MINUTE) * 60;
	sum += Chip_RTC_GetTime(LPC_RTC,RTC_TIMETYPE_SECOND);
	Chip_RTC_Enable(LPC_RTC, ENABLE);			//START RTC
	return sum;
}


void get_clock_imm(tstamp *tnow){
	//
	tnow->s = get_curr_time();
	tnow->fs = ((SysTick_GetMS() % 1000) + systick_ms_offset)*MILISECOND_TO_FRACTION;
	return;
}

tstamp *sub_u64(tstamp *min, tstamp *sup, tstamp *res){
	asm volatile(
			"subs %[minfs], %[supfs]\n\t"
			"sbc %[mins], %[sups]\n\t"
			"mov %[resfs], %[minfs]\n\t"
			"mov %[ress], %[mins]"
			:[ress]"=r" (res->s), [resfs]"=r" (res->fs)
			:[mins]"r" (min->s), [minfs]"r" (min->fs), [sups]"r" (sup->s), [supfs]"r" (sup->fs)
			:"cc","memory");
	return res;
}

tstamp *add_u64(tstamp *zx, tstamp *zy, tstamp *sum){
	asm volatile(
			"adds %[z1fs], %[z2fs]\n\t"
			"adc %[z1s], %[z2s]\n\t"
			"mov %[res_fs], %[z1fs]\n\t"
			"mov %[res_s], %[z1s]"
			:[res_s]"=r" (sum->s), [res_fs]"=r" (sum->fs)
			:[z1s]"r" (zx->s), [z1fs]"r" (zx->fs), [z2s]"r" (zy->s), [z2fs]"r" (zy->fs)
			:"cc","memory");
	return sum;
}


tstamp *div2_u64(tstamp *ts){
	asm volatile(
		"mrs r6, APSR\n\t"
		"mov r5, #0x20000000\n\t"
		"bic r6, r5\n\t"			//set carry to '0'
		"msr APSR_nzcvq, r6\n\t"
		"asrs %[s], #1\n\t"
		"rrx r6, %[fs]\n\t"
		"mov %[sout], %[s]\n\t"
		"mov %[fsout], r6"
		:[sout]"=r" (ts->s), [fsout]"=r" (ts->fs)
		:[s]"r" (ts->s), [fs]"r" (ts->fs)
		:"cc","memory","r6", "r5");
	return ts;
}


uint32_t htons_32(uint32_t in){
	asm volatile(
			"rev %[o], %[i]"
			:[o]"=r" (in)
			:[i]"r" (in)
			:"cc","memory");
	return in;
}


uint16_t htons_16(uint16_t in){
	asm volatile(
			"rev16 %[o], %[i]\n\t"
			:[o]"=r" (in)
			:[i]"r" (in)
			:"cc","memory");
	return in & 0xffff;
}

//according to RFC5905:   theta = T(B) - T(A) = 1/2 * [(T2-T1) + (T3-T4)]

void ntp_offset(ntp_msg *p, tstamp *dst, tstamp *theta){
	tstamp ts1, ts2, diff1, diff2;

	//computing T2-T1
	ts2.fs = htons_32(p->rec.fs);
	ts2.s = htons_32(p->rec.s);
	ts1.fs = htons_32(p->org.fs);
	ts1.s = htons_32(p->org.s);
	sub_u64(&ts2, &ts1, &diff1);
	//computing T3-T4
	ts2.fs = dst->fs;
	ts2.s = dst->s;
	ts1.fs = htons_32(p->xmt.fs);
	ts1.s = htons_32(p->xmt.s);
	sub_u64(&ts1, &ts2, &diff2);
	//sum diff1/2 and diff2/2
	div2_u64(&diff1);
	div2_u64(&diff2);
	add_u64(&diff1,&diff2,theta);
//	if((theta->fs > 0x40000000) || (theta->s != 0)) printf("correct %u %u\n", theta->s, theta->fs);
}

void create_txmsg(ntp_msg *m){
	tstamp tnow;
	m->leap_ver_mode =  0 | (NTP_VERSION<< 3) | NTPMODE_CLNT;
	m->stratum = 3;
	m->poll = 10;
	m->precision = NTP_PRECISION;
	m->rootdelay.s = htons_16(sntp.delay.s);
	m->rootdelay.fs = htons_16(sntp.delay.fs);
	m->rootdisp.s = 0;
	m->rootdisp.fs = 0;
	m->refid = sntp.refid.addr;
	m->reftime.fs = htons_32(sntp.ts_reference.fs);
	m->reftime.s = htons_32(sntp.ts_reference.s);
	m->org.fs = htons_32(sntp.ts_org.fs);
	m->org.s = htons_32(sntp.ts_org.s);
	m->rec.fs = htons_32(sntp.ts_rec.fs);
	m->rec.s = htons_32(sntp.ts_rec.s);
	get_clock_imm(&tnow);
	m->xmt.fs = htons_32(tnow.fs);
	m->xmt.s = htons_32(tnow.s);
	sntp.ts_xmt.fs = tnow.fs;
	sntp.ts_xmt.s = tnow.s;
	return;
}


void correct_curr_time(uint32_t s, uint32_t fs){
	uint32_t ctime;
	//to get real miliseconds time we add to or subtract from
	//an offset from systick clock.
	//uint32_t systick_ms_offset_old = systick_ms_offset;
	//uint32_t systickms = Systick_GetMS()%1000;
	fs += (systick_ms_offset*MILISECOND_TO_FRACTION);
	systick_ms_offset = fs/MILISECOND_TO_FRACTION;
	ctime = get_curr_time();
	ctime += s;						//correct seconds
	set_curr_time(ctime);
	return;
}


void  parse_ntp_msg(ntp_msg *ntpmsg, struct udp_pcb *ntp_pcb, tstamp *trec){
	tstamp theta,tnow;
	uint32_t kod, c;
	char *p;
	//checking for kiss of death code
	if(ntpmsg->stratum == 0){
		//copy msg's ref id to our memory
		kod = htons_32(ntpmsg->refid);
		p = (char *) &kod;
		if(*p == 'R'){
			if(*(p+1) == 'A'){
				//checking KOD "RATE"
				for(c = 0; c < 4; c++)
					if(*(p+c) != str_kod[11][c]) break;
				//if( c == 4) interval_inc();

			}
			else if (*(p+1) == 'S'){
				//checking KOD "RSSTR"
				for(c = 0; c < 4; c++)
					if(*(p+c) != str_kod[7][c]) break;
				//if( c == 4) interval_inc();

			}
		}
		else if   (*p == 'D'){
			//checking KOD "DENY"
			for(c = 0; c < 4; c++)
				if(*(p+c) != str_kod[5][c]) break;
			//if( c == 4) interval_inc();

		}
	}
	//checking correct ntp message
	if(((ntpmsg->org.fs == htons_32(sntp.ts_xmt.fs)) ||
			(ntpmsg->org.s == htons_32(sntp.ts_xmt.s))) &&
			((ntpmsg->xmt.fs != htons_32(sntp.ts_org.fs)) ||
			(ntpmsg->xmt.s != htons_32(sntp.ts_org.s)))){
		tstamp min,sup,res1,res2;
		//ntp packet is correct
		sntp.ts_dst.fs = trec->fs;
		sntp.ts_dst.s = trec->s;
		//calculate delay epsilon = (T4-T1) - (T3-T2)
		//T4-T1
		min.fs = sntp.ts_dst.fs;
		min.s = sntp.ts_dst.s;
		sup.fs = htons_32(ntpmsg->org.fs);
		sup.s = htons_32(ntpmsg->org.s);
		sub_u64(&min,&sup,&res1);
		//T3-T2
		min.fs = htons_32(ntpmsg->xmt.fs);
		min.s = htons_32(ntpmsg->xmt.s);
		sup.fs = htons_32(ntpmsg->rec.fs);
		sup.s = htons_32(ntpmsg->rec.s);
		sub_u64(&min,&sup,&res2);
		//1-st sub is in sntp.delay -> min
		//2nd sub is in res			-> sup
		min.fs = res1.fs;
		min.s = res1.s;
		sup.fs = res2.fs;
		sup.s = res2.s;
		sub_u64(&min,&sup,&res1);
		//end
		sntp.delay.fs = (uint16_t) (res1.fs >> 16);
		sntp.delay.s = (uint16_t) res1.s & 0xFFFF;
		//servers tx -> org
		sntp.ts_org.fs = htons_32(ntpmsg->xmt.fs);
		sntp.ts_org.s = htons_32(ntpmsg->xmt.s);
		//dst -> rec
		sntp.ts_rec.fs = sntp.ts_dst.fs;
		sntp.ts_rec.s = sntp.ts_dst.s;
		ntp_offset(ntpmsg, trec, &theta);
		correct_curr_time(theta.s, theta.fs);
		get_clock_imm(&tnow);
		sntp.ts_reference.fs = tnow.fs;
		sntp.ts_reference.s = tnow.s;
		ip_addr_set(&sntp.refid, &ntp_pcb->remote_ip);
	}

}

void ntp_init(){
	sntp.ticks = CLOCK_SECOND*30;
}

static void ntp_recv(void *arg, struct udp_pcb *ntppcb, struct pbuf *p, ip_addr_t *addr, u16_t port){
	tstamp now;
	ntp_msg *m = p->payload;
	now.fs = ((SysTick_GetMS() % 1000)+systick_ms_offset)*MILISECOND_TO_FRACTION;
	now.s = get_curr_time();
	parse_ntp_msg(p->payload,ntppcb, &now);
	pbuf_free(p);
	if(ntppcb != NULL){
		udp_remove(ntppcb);
	}
}



void ntp_dns_found(const char *hostname, ip_addr_t *ipaddr, void *arg){
	err_t err;
	struct udp_pcb *pcb = (struct udp_pcb *)arg;
	//send message
	struct pbuf *p;
	ntp_msg *m;

	p = pbuf_alloc(PBUF_TRANSPORT, sizeof(ntp_msg),PBUF_RAM);
	if(p != NULL){
		create_txmsg(p->payload);
		udp_connect(pcb, ipaddr, NTP_PORT);
		err = udp_sendto(pcb, p, ipaddr, NTP_PORT);
		//store some data into the sntp object
		m = (ntp_msg *)p->payload;
		sntp.ts_org.fs = m->org.fs;
		sntp.ts_org.s = m->org.s;
		/*free pbuf*/
		pbuf_free(p);
	}

}



err_t ntp_start(){
	err_t err;
	ip_addr_t addr;
	struct udp_pcb *ntp_pcb;

	ntp_pcb = udp_new();

	if(ntp_pcb != NULL){
		udp_bind(ntp_pcb,IP_ADDR_ANY,NTP_PORT);
		udp_recv(ntp_pcb,ntp_recv,ntp_pcb);

	}
	else {
		err = ERR_MEM;
		return err;
	}
	err = dns_gethostbyname(ntp_server_name,&addr,ntp_dns_found,ntp_pcb);
	//ip address is found-> we don't need to ask DNS service
	if(err == ERR_OK){
		ntp_dns_found(ntp_server_name, &addr, ntp_pcb);
	}
	return err;
}



