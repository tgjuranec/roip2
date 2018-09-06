/*
===============================================================================
 Name        : roip.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
/*
 *blablabla
 * */

#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "ipv4/lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

#include "lpc_phy.h"
#include "arch/lpc17xx_40xx_emac.h"
#include "arch/lpc_arch.h"
// TODO: insert other include files here
#include <string.h>

#include <adc.h>
#include <http.h>
#include <speex_udp.h>
#include <serial.h>
#include <parser.h>
#include <iap.h>
// TODO: insert other definitions and declarations here

/* NETIF data */
struct netif lpc_netif;



/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* LED0 is used for the link status, on = PHY cable detected */
	/* Initial LED state is off to show an unconnected cable state */
	Board_LED_Set(0, false);

	/* Setup a 1mS sysTick for the primary time base */
	SysTick_Enable(1);
}



struct flash_data *fd;


int main(void) {


#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
    ip_addr_t ipaddr, netmask, gw, server1_ip,server2_ip;
	uint32_t physts;

	static int prt_ip = 0;

	prvSetupHardware();
	ADC_init(0,25,ADC_CH2,8000,NULL,NULL);
	serial_init();
	/* Initialize LWIP */

	lwip_init();
	sys_timeouts_init();
	LWIP_DEBUGF(LWIP_DBG_ON, ("Starting LWIP TCP echo server...\n"));

	/* Static IP assignment */
#if LWIP_DHCP
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
	IP4_ADDR(&gw, 192, 168, 1, 1);
	IP4_ADDR(&ipaddr, 192, 168, 1, 201);
	IP4_ADDR(&netmask, 255, 255, 255, 0);

#endif
	IP4_ADDR(&server1_ip, 192, 168, 1, 198);
	IP4_ADDR(&server2_ip, 192, 168, 1, 197);

	/* READ FLASH MEMORY
	 * IP assignment from flash memory
	 * get MAC address and assign to MAC layer
	 * get password
	 *
	 */

	get_flash();
	char *pass;
	fd = (struct flash_data *) flash_buff;
	if(ip_addr_netcmp(&fd->ip_local,&fd->ip_gw,&fd->ip_nm) \
			&&(ip_addr_netcmp(&fd->ip_local,&fd->ser1,&fd->ip_nm) \
				|| ip_addr_netcmp(&fd->ip_local,&fd->ser2,&fd->ip_nm))){
		ip_addr_copy(ipaddr,fd->ip_local);
		ip_addr_copy(gw,fd->ip_gw);
		ip_addr_copy(netmask,fd->ip_nm);
		ip_addr_copy(server1_ip,fd->ser1);
		ip_addr_copy(server2_ip,fd->ser2);
	}
	pass = &fd->password[0];


	/* Add netif interface for lpc17xx_8x */
	netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
			  ethernet_input);
	netif_set_default(&lpc_netif);
	netif_set_up(&lpc_netif);

#if LWIP_DHCP
	dhcp_start(&lpc_netif);
#endif
	spudp_init(&ipaddr,&server1_ip, &server2_ip);
	http_init();
	radio_comm_init();

	/* This could be done in the sysTick ISR, but may stay in IRQ context
	   too long, so do this stuff with a background loop. */
	while (1) {
		/* Handle packets as part of this loop, not in the IRQ handler */
		lpc_enetif_input(&lpc_netif);

		/* lpc_rx_queue will re-qeueu receive buffers. This normally occurs
		   automatically, but in systems were memory is constrained, pbufs
		   may not always be able to get allocated, so this function can be
		   optionally enabled to re-queue receive buffers. */
#if 0
		while (lpc_rx_queue(&lpc_netif)) {}
#endif

		/* Free TX buffers that are done sending */
		lpc_tx_reclaim(&lpc_netif);

		/* LWIP timers - ARP, DHCP, TCP, etc. */
		sys_check_timeouts();

		/* Call the PHY status update state machine once in a while
		   to keep the link status up-to-date */
		physts = lpcPHYStsPoll();

		/* Only check for connection state when the PHY status has changed */
		if (physts & PHY_LINK_CHANGED) {
			if (physts & PHY_LINK_CONNECTED) {
				Board_LED_Set(0, true);
				prt_ip = 0;

				/* Set interface speed and duplex */
				if (physts & PHY_LINK_SPEED100) {
					Chip_ENET_Set100Mbps(LPC_ETHERNET);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 100000000);
				}
				else {
					Chip_ENET_Set10Mbps(LPC_ETHERNET);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 10000000);
				}
				if (physts & PHY_LINK_FULLDUPLX) {
					Chip_ENET_SetFullDuplex(LPC_ETHERNET);
				}
				else {
					Chip_ENET_SetHalfDuplex(LPC_ETHERNET);
				}

				netif_set_link_up(&lpc_netif);
			}
			else {
				Board_LED_Set(0, false);
				netif_set_link_down(&lpc_netif);
			}

			DEBUGOUT("Link connect status: %d\r\n", ((physts & PHY_LINK_CONNECTED) != 0));
		}


		//listening UART connection
		radio_comm();
		//check connection with server
		spudp_control_send();

		if(n_conv_finished){
			if(spudp_send(ssi,gssi, ssi2,block_result,BLOCK_SIZE)!= BLOCK_SIZE){

			}
			n_conv_finished = 0;
		}
		if(sysreset_req){
			if((sys_now()- sysreset_req) > 500)
				NVIC_SystemReset();
		}



	}
#endif
#endif


    return 0 ;
}
