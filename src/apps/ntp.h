/*
 * ntp.h
 *
 *  Created on: 25.9.2011.
 *      Author: djuka
 */

#ifndef NTP_H_
#define NTP_H_
#include "../lwip/inc/ipv4/lwip/ip_addr.h"
#include "../lwip/inc/lwip/err.h"

/*
               +-----------+------------+-----------------------+
               | Name      | Formula    | Description           |
               +-----------+------------+-----------------------+
               | leap      | leap       | leap indicator (LI)   |
               | version   | version    | version number (VN)   |
               | mode      | mode       | mode                  |
               | stratum   | stratum    | stratum               |
               | poll      | poll       | poll exponent         |
               | precision | rho        | precision exponent    |
               | rootdelay | delta_r    | root delay            |
               | rootdisp  | epsilon_r  | root dispersion       |
               | refid     | refid      | reference ID          |
               | reftime   | reftime    | reference timestamp   |
               | org       | T1         | origin timestamp      |
               | rec       | T2         | receive timestamp     |
               | xmt       | T3         | transmit timestamp    |
               | dst       | T4         | destination timestamp |
               | keyid     | keyid      | key ID                |
               | dgst      | dgst       | message digest        |
               +-----------+------------+-----------------------+
*/
/*
 * Timestamp conversion macroni
 */



#define FRIC        65536.                  /* 2^16 as a double */
#define D2FP(r)     ((tdist)((r) * FRIC))   /* NTP short */
#define FP2D(r)     ((double)(r) / FRIC)

#define FRAC       4294967296.             /* 2^32 as a double */
#define D2LFP(a)   ((tstamp)((a) * FRAC))  /* NTP timestamp */
#define LFP2D(a)   ((double)(a) / FRAC)
#define U2LFP(a)   (((unsigned long long) \
                       ((a).tv_sec + JAN_1970) << 32) + \
                       (unsigned long long) \
                       ((a).tv_usec / 1e6 * FRAC))

/*
 * Arithmetic conversions
 */
#define LOG2D(a)        ((a) < 0 ? 1. / (1L << -(a)) : \
                            1L << (a))          /* poll, etc. */
#define SQUARE(x)       (x * x)
#define SQRT(x)         (sqrt(x))

/*
 * Global constants.  Some of these might be converted to variables
 * that can be tinkered by configuration or computed on-the-fly.  For
 * instance, the reference implementation computes PRECISION on-the-fly
 * and provides performance tuning for the defines marked with % below.
 */
#define NTP_VERSION         4       /* version number */
#define MINDISP         .01     /* % minimum dispersion (s) */
#define MAXDISP         16      /* maximum dispersion (s) */
#define MAXDIST         1       /* % distance threshold (s) */
#define NOSYNC          0x3     /* leap unsync */
#define MAXSTRAT        16      /* maximum stratum (infinity metric) */
#define MINPOLL         6       /* % minimum poll interval (64 s)*/
#define MAXPOLL         17      /* % maximum poll interval (36.4 h) */
#define MINCLOCK        3       /* minimum manycast survivors */
#define MAXCLOCK        10      /* maximum manycast candidates */
#define TTLMAX          8       /* max ttl manycast */
#define BEACON          15      /* max interval between beacons */

#define PHI             15e-6   /* % frequency tolerance (15 ppm) */
#define NSTAGE          8       /* clock register stages */
#define NMAX            50      /* maximum number of peers */
#define NSANE           1       /* % minimum intersection survivors */
#define NMIN            3       /* % minimum cluster survivors */





/*
 * Local clock process return codes
 */
#define IGNORE          0       /* ignore */
#define SLEW            1       /* slew adjustment */
#define STEP            2       /* step adjustment */
#define PANIC           3       /* panic - no adjustment */

/*
 * System flags
 */
#define S_FLAGS         0       /* any system flags */
#define S_BCSTENAB      0x1     /* enable broadcast client */

/*
 * Peer flags
 */
#define P_FLAGS         0       /* any peer flags */
#define P_EPHEM         0x01    /* association is ephemeral */
#define P_BURST         0x02    /* burst enable */
#define P_IBURST        0x04    /* intial burst enable */
#define P_NOTRUST       0x08    /* authenticated access */
#define P_NOPEER        0x10    /* authenticated mobilization */
#define P_MANY          0x20    /* manycast client */


/*
 * Authentication codes
 */
#define A_NONE          0       /* no authentication */
#define A_OK            1       /* authentication OK */
#define A_ERROR         2       /* authentication error */
#define A_CRYPTO        3       /* crypto-NAK */

/*
 * Association state codes
 */
#define X_INIT          0       /* initialization */
#define X_STALE         1       /* timeout */
#define X_STEP          2       /* time step */
#define X_ERROR         3       /* authentication error */
#define X_CRYPTO        4       /* crypto-NAK received */
#define X_NKEY          5       /* untrusted key */

/*
 * Protocol mode definitions
 */
#define NTPMODE_RSVD          0       /* reserved */
#define NTPMODE_SACT          1       /* symmetric active */
#define NTPMODE_PASV          2       /* symmetric passive */
#define NTPMODE_CLNT          3       /* client */
#define NTPMODE_SERV          4       /* server */
#define NTPMODE_BCST          5       /* broadcast server */
#define NTPMODE_BCLN          6       /* broadcast client */

/*
 * Clock state definitions
 */
#define NSET            0       /* clock never set */
#define FSET            1       /* frequency set from file */
#define SPIK            2       /* spike detected */
#define FREQ            3       /* frequency mode */
#define CLOCKSYNC            4       /* clock synchronized */

#define min(a, b)   ((a) < (b) ? (a) : (b))
#define max(a, b)   ((a) < (b) ? (b) : (a))

#define NTP_PRECISION       -10     /* precision (log2 s)  */

/*ntp state*/
#define NTP_UNINIT	0	//ntp uninitializied
#define NTP_INIT	1	//ntp initialization over
#define NTP_REQSEND	2	//ntp request send
#define NTP_ANSRECVD	3	//ntp answer received


typedef struct td_tstamp{
	uint32_t s;
	uint32_t fs;
} tstamp;

typedef struct td_tdiffstamp{
	int32_t s;
	int32_t fs;
} tdiffstamp;

typedef struct td_short_tstamp{
	uint16_t s;
	uint16_t fs;
} short_tstamp;

typedef struct td_date{
	uint16_t year;
	uint8_t month;
	uint8_t day;
} date;



typedef struct td_ntp_msg {
		uint8_t    leap_ver_mode;           /* leap indicator - 2 bits*/
										/* version number - 3 bits*/
										/* mode - 3 bits */
		uint8_t    stratum;        /* stratum */
		uint8_t    poll;           /* poll interval */
        uint8_t  precision;      /* precision */
        short_tstamp rootdelay;      /* root delay */
        short_tstamp rootdisp;       /* root dispersion */
        uint32_t    refid;          /* reference ID */
        tstamp  reftime;        /* reference time */
        tstamp  org;            /* origin timestamp */
        tstamp  rec;            /* receive timestamp */
        tstamp  xmt;            /* transmit timestamp */
        int     keyid;          /* key ID */
        int  mac[4];            /* message digest */
} ntp_msg;



struct ntp_state {
	uint8_t count;
	uint16_t ticks;
	ip_addr_t refid;		//in network byte order
	short_tstamp delay;		//or delta
	short_tstamp disp;		//or epsilon
	tstamp ts_org;
	tstamp ts_rec;
	tstamp ts_xmt;
	tstamp ts_dst;
	tstamp ts_reference;
};


void ntp_init();
err_t ntp_start();
uint32_t get_curr_time();


#endif /* NTP_H_ */
