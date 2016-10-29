/*
 * serial.c
 *
 *  Created on: Sep 12, 2016
 *      Author: tgjuranec
 */
#include <board.h>
#include <assert.h>


RINGBUFF_T rxring,txring;
static uint8_t rxbuff[256], txbuff[192];

static void UART_init(LPC_USART_T *pUART, IRQn_Type IRQn, uint32_t baudrate,uint32_t config, uint32_t rx_fifo_int_level)
{
	Chip_UART_Init(pUART);
	Chip_UART_SetBaud(pUART, baudrate);
	Chip_UART_ConfigData(pUART, config);
	Chip_UART_TXEnable(pUART);
	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(pUART, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | rx_fifo_int_level));
	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(pUART, (UART_IER_RBRINT | UART_IER_RLSINT));
	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(IRQn, 1);
	NVIC_EnableIRQ(IRQn);
}

void serial_init(){
	//UART1 pins
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,0,15);
	Chip_IOCON_PinMux(LPC_IOCON,0,15,0,IOCON_FUNC1);
	Chip_IOCON_PinMux(LPC_IOCON,0,16,0,IOCON_FUNC1);
	//UART1 configuration
	UART_init(LPC_UART1,UART1_IRQn,9600,UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS, UART_FCR_TRG_LEV3);
	//UART_init(LPC_UART1,UART1_IRQn,4800,UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS, UART_FCR_TRG_LEV3);
	//init dedicated ringbuffer
	RingBuffer_Init(&rxring, rxbuff, 1, 256);
	RingBuffer_Init(&txring, txbuff, 1, 192);
}



int serial_send(const uint8_t *buf, int len){
	assert(buf != NULL);
	assert(len > 0);
	assert(len <= 192);
	if(RingBuffer_GetFree(&txring) < len) return 0;	//there's not enough space
	//move the data
	int received_data = RingBuffer_InsertMult(&txring,buf,len);
	//start first interrupt
	Chip_UART_IntEnable(LPC_UART1, UART_IER_THREINT);
	NVIC->STIR = 0X06;
	return received_data;
}


int serial_nitems_RB(){
	return RingBuffer_GetCount(&rxring);
}



int serial_read(uint8_t *buf, int len){
	assert(buf != NULL);
	assert(len > 0);
	int ret = RingBuffer_PopMult(&rxring,buf,len);
	return ret;
}


/**
 * @brief	UART 1 interrupt handler using ring buffers
 * @return	Nothing
 */
void UART1_IRQHandler(void)
{
	NVIC_DisableIRQ(UART1_IRQn);
	uint32_t irq_pending = Chip_UART_ReadIntIDReg(LPC_UART1);
	uint32_t line_status = Chip_UART_ReadLineStatus(LPC_UART1);
	if(RingBuffer_GetCount(&txring)>0){
		Chip_UART_TXIntHandlerRB(LPC_UART1, &txring);
		if(RingBuffer_IsEmpty(&txring)){ //msg done
			RingBuffer_Flush(&txring);
			Chip_UART_IntDisable(LPC_UART1, UART_IER_THREINT);
			//UART1_TX_end++;
		}
	}
	if(!(irq_pending & UART_IIR_INTSTAT_PEND)){
		//READ LINE STATUS
		if((irq_pending & UART_IIR_INTID_MASK)==UART_IIR_INTID_RLS){
			//if error on line -> destroy data in buffer
			if(line_status & (UART_LSR_PE | UART_LSR_FE | UART_LSR_BI)){
				//UART1_RX_end++;
			}
			else if(line_status & UART_LSR_OE){
				//UART1_RX_end++;
			}

		}
		//RX DATA AVAILABLE
		if((irq_pending & UART_IIR_INTID_MASK)==UART_IIR_INTID_RDA){
			//if(UART1_RX_start == 0) UART1_RX_start++;
			Chip_UART_RXIntHandlerRB(LPC_UART1,&rxring);
			if(RingBuffer_IsFull(&rxring)){
				//UART1_RX_end++;
			}
		}
		//CHARACTER TIME OUT
		if((irq_pending & UART_IIR_INTID_MASK)==UART_IIR_INTID_CTI){
			if(line_status & UART_LSR_RDR){
				Chip_UART_RXIntHandlerRB(LPC_UART1,&rxring);
			}
			//UART1_RX_end++;
		}
		//tmr_rxmsg = TimeTick; version 0.71
	}
	NVIC_EnableIRQ(UART1_IRQn);
}
