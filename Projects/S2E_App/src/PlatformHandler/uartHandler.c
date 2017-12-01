#include <string.h>
#include "W7500x_uart.h"
#include "W7500x_gpio.h"
#include "common.h"
#include "W7500x_board.h"
#include "configdata.h"
#include "uartHandler.h"
#include "seg.h"

#include <stdio.h> // for debugging

/* Private typedef -----------------------------------------------------------*/
RINGBUFF_T txring[DEVICE_UART_CNT];
RINGBUFF_T rxring[DEVICE_UART_CNT];

/* Private define ------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
extern void delay(__IO uint32_t nCount);

/* Private functions ---------------------------------------------------------*/
int32_t uart_putc(uint8_t uartNum, uint8_t ch);
int32_t uart_puts(uint8_t uartNum, uint8_t* buf, uint16_t reqSize);
int32_t uart_getc(uint8_t uartNum);
int32_t uart_getc_nonblk(uint8_t uartNum);
//int32_t uart_gets(uint8_t uartNum, uint8_t* buf, uint16_t reqSize);

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t flag_ringbuf_full[DEVICE_UART_CNT] = {0,};

static uint8_t rxbuff[DEVICE_UART_CNT][UART_RRB_SIZE];
static uint8_t txbuff[DEVICE_UART_CNT][UART_SRB_SIZE];

uint8_t word_len_table[] = {7, 8, 9};
uint8_t * parity_table[] = {(uint8_t *)"N", (uint8_t *)"ODD", (uint8_t *)"EVEN"};
uint8_t stop_bit_table[] = {1, 2};
uint8_t * flow_ctrl_table[] = {(uint8_t *)"NONE", (uint8_t *)"XON/XOFF", (uint8_t *)"RTS/CTS", (uint8_t *)"RTS Only"};
uint8_t * uart_if_table[] = {(uint8_t *)UART_IF_STR_RS232_TTL, (uint8_t *)UART_IF_STR_RS422_485};

// XON/XOFF Status; 
static uint8_t xonoff_status[DEVICE_UART_CNT] = {UART_XON,};

// RTS Status; __USE_GPIO_HARDWARE_FLOWCONTROL__ defined
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
	static uint8_t rts_status[DEVICE_UART_CNT] = {UART_RTS_LOW,};
#endif

// UART Interface selecter; RS-422 or RS-485 use only
static uint8_t uart_if_mode[DEVICE_UART_CNT] = {UART_IF_RS422,};

/* Public functions ----------------------------------------------------------*/

void S2E_UART_IRQ_Handler(UART_TypeDef * s2e_uart)
{
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    
	uint8_t ch; // 1-byte character variable for UART Interrupt request handler
    uint8_t channel = (s2e_uart == UART0)? 0 : 1;

	if(UART_GetITStatus(s2e_uart,  UART_IT_FLAG_RXI))
	{
        if(RingBuffer_IsFull(&rxring[channel]))
		{
			UART_ReceiveData(s2e_uart);
			
			flag_ringbuf_full[channel] = 1;

		}
		else
		{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
			if(serial_option[channel].flow_control == flow_rts_cts)
			{
				;
			}
#else
			//if((serial_option[channel].flow_control == flow_rts_cts) && (M_BUFFER_USED_SIZE(channel) > UART_OFF_THRESHOLD)) // CTS/RTS
            if((serial_option[channel].flow_control == flow_rts_cts) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD)) // CTS/RTS
			{
				; // Does nothing => RTS signal inactive
			}
			else
#endif
			{
				//ch = UartGetc(s2e_uart);
				ch = UART_ReceiveData(s2e_uart);
#ifdef _SEG_DEBUG_
				UART_SendData(s2e_uart, ch);	// ## UART echo; for debugging
#endif
                if(channel==0)
                {
                    if((check_modeswitch_trigger(ch))==0) // ret: [0] data / [1] trigger code
                    {
                        if(check_serial_store_permitted(channel, ch)==1) // ret: [0] not permitted / [1] permitted
                        {
 
                            /*BUFFER_IN(data_rx_0)=ch;
                            BUFFER_IN_MOVE(data_rx_0, 1);*/
                            RingBuffer_Insert(&rxring[channel], &ch);
                        }
                    }
                }
                else
                {
                    if(check_serial_store_permitted(channel, ch)==1) // ret: [0] not permitted / [1] permitted
                    {
                        /*BUFFER_IN(data_rx_1)=ch;
                        BUFFER_IN_MOVE(data_rx_1, 1);*/
                        RingBuffer_Insert(&rxring[channel], &ch);
                    }
                }
			}
		}
		init_time_delimiter_timer(channel);
		
		UART_ClearITPendingBit(s2e_uart, UART_IT_FLAG_RXI);
	}

	// Does not use: UART Tx interrupt
	if(UART_GetITStatus(s2e_uart, UART_IT_FLAG_TXI)) 
	{
        Chip_UART_TXIntHandlerRB(s2e_uart, &txring[channel]);
		UART_ClearITPendingBit(s2e_uart, UART_IT_FLAG_TXI);
	}

}
void Chip_UART_TXIntHandlerRB(UART_TypeDef *pUART, RINGBUFF_T *pTXRB)
{
    uint8_t ch_tx;
    
    if(RingBuffer_Pop(pTXRB, &ch_tx))
    {
        UART_SendData(pUART, ch_tx);
    }
    else												// RingBuffer Empty
    {
        UART_ITConfig(pUART, UART_IT_FLAG_TXI, DISABLE);
    }    
}
uint32_t Chip_UART_SendRB(UART_TypeDef *pUART, RINGBUFF_T *pRB, const void *data, uint16_t bytes)
{
	uint32_t ret;
	uint8_t *p8 = (uint8_t *) data;
	uint8_t ch;

	/* Don't let UART transmit ring buffer change in the UART IRQ handler */
	//UART_ITConfig(pUART, UART_IT_FLAG_TXI, DISABLE);

	/* Move as much data as possible into transmit ring buffer */
	ret = RingBuffer_InsertMult(pRB, p8, bytes);

	/* Enable UART transmit interrupt */
    
	UART_ITConfig(pUART, UART_IT_FLAG_TXI, ENABLE);

	if(RingBuffer_Pop(pRB, &ch))
	{
		UART_SendData(pUART, ch);
	}
	//UART_SendData(pUART, 0x0a);

	return ret;
}
void UART_buffer_flush(RINGBUFF_T *buf)
{
	RingBuffer_Flush(buf);
}

void S2E_UART_Configuration(uint8_t channel)
{
    UART_InitTypeDef UART_InitStructure;
 
    if(channel)
    {
        /* Ring Buffer */
        RingBuffer_Init(&rxring[channel], rxbuff[channel], 1, UART_RRB_SIZE);
        RingBuffer_Init(&txring[channel], txbuff[channel], 1, UART_SRB_SIZE);
        /* Configure the UART1 */
        serial_info_init(&UART_InitStructure, channel);
        UART_Init(UART1,&UART_InitStructure);
        /* Configure UART1 Interrupt Enable */
        UART_ITConfig(UART1, (UART_IT_FLAG_TXI | UART_IT_FLAG_RXI), ENABLE);
        /* NVIC configuration */
        NVIC_ClearPendingIRQ(UART1_IRQn);
        NVIC_SetPriority(UART1_IRQn, 1);
        NVIC_EnableIRQ(UART1_IRQn);
    }
    else
    {
        /* Ring Buffer */
        RingBuffer_Init(&rxring[channel], rxbuff[channel], 1, UART_RRB_SIZE);
        RingBuffer_Init(&txring[channel], txbuff[channel], 1, UART_SRB_SIZE);
        /* Configure the UART0 */
        serial_info_init(&UART_InitStructure, channel);
        UART_Init(UART0,&UART_InitStructure);
        /* Configure UART0 Interrupt Enable */
        UART_ITConfig(UART0, (UART_IT_FLAG_TXI | UART_IT_FLAG_RXI), ENABLE);
        /* NVIC configuration */
        NVIC_ClearPendingIRQ(UART0_IRQn);
        NVIC_SetPriority(UART0_IRQn, 1);
        NVIC_EnableIRQ(UART0_IRQn);
    }
}

void UART2_Configuration(void)
{
	/* Configure UART2: Simple UART */
	/* UART2 configured as follow: 115200-8-N-1 */
	
	S_UART_Init(115200);
}

void serial_info_init(UART_InitTypeDef* UART_InitStructure, uint8_t channel)
{
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    
	uint32_t valid_arg = 0;
	uint32_t baud_table[14] = {300, 600, 1200, 1800, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400};

	/* Set Baud Rate */
	if(serial_option[channel].baud_rate < (sizeof(baud_table) / sizeof(baud_table[0])))
	{
		UART_InitStructure->UART_BaudRate = baud_table[serial_option[channel].baud_rate];
		valid_arg = 1;
	}
	
	if(!valid_arg)
		UART_InitStructure->UART_BaudRate = baud_table[baud_115200];

	/* Set Data Bits */
	switch(serial_option[channel].data_bits) {
		case word_len7:
			UART_InitStructure->UART_WordLength = UART_WordLength_7b;
			break;
		case word_len8:
			UART_InitStructure->UART_WordLength = UART_WordLength_8b;
			break;
		default:
			UART_InitStructure->UART_WordLength = UART_WordLength_8b;
			serial_option[channel].data_bits = word_len8;
			break;
	}

	/* Set Stop Bits */
	switch(serial_option[channel].stop_bits) {
		case stop_bit1:
			UART_InitStructure->UART_StopBits = UART_StopBits_1;
			break;
		case stop_bit2:
			UART_InitStructure->UART_StopBits = UART_StopBits_2;
			break;
		default:
			UART_InitStructure->UART_StopBits = UART_StopBits_1;
			serial_option[channel].stop_bits = stop_bit1;
			break;
	}

	/* Set Parity Bits */
	switch(serial_option[channel].parity) {
		case parity_none:
			UART_InitStructure->UART_Parity = UART_Parity_No;
			break;
		case parity_odd:
			UART_InitStructure->UART_Parity = UART_Parity_Odd;
			break;
		case parity_even:
			UART_InitStructure->UART_Parity = UART_Parity_Even;
			break;
		default:
			UART_InitStructure->UART_Parity = UART_Parity_No;
			serial_option[channel].parity = parity_none;
			break;
	}
	
	/* Flow Control */
	if(serial_option[channel].uart_interface == UART_IF_RS232_TTL)
	{
		// RS232 Hardware Flow Control
		//7		RTS		Request To Send		Output
		//8		CTS		Clear To Send		Input
		switch(serial_option[channel].flow_control) {
			case flow_none:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
			case flow_rts_cts:
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
                UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				if(channel)
				{
					GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_OUT, UART1_RTS_PAD_AF);
					GPIO_Configuration(UART1_CTS_PORT, UART1_CTS_PIN, GPIO_Mode_IN, UART1_CTS_PAD_AF);
					set_uart_rts_pin_low(channel);
			 	}
				else
				{
					GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_OUT, UART0_RTS_PAD_AF);
					GPIO_Configuration(UART0_CTS_PORT, UART0_CTS_PIN, GPIO_Mode_IN, UART0_CTS_PAD_AF);
					set_uart_rts_pin_low(channel);
				}
#else
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_RTS_CTS;
#endif
				break;
			case flow_xon_xoff:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
			default:
				UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
				break;
		}
	}
	else // UART_IF_RS422_485
	{
		UART_InitStructure->UART_HardwareFlowControl = UART_HardwareFlowControl_None;
		
        // GPIO configration (RTS pin -> GPIO: 485SEL)
		if((serial_option[channel].flow_control != flow_rtsonly) && (serial_option[channel].flow_control != flow_reverserts)) // Added by James in March 29
		{
			get_uart_rs485_sel(SEG_DATA_UART0);
		}
        else	// Added by James in March 29
		{
			if(serial_option->flow_control == flow_rtsonly)
			{
				uart_if_mode[channel] = UART_IF_RS485;
			}
            else
			{
				uart_if_mode[channel] = UART_IF_RS485_REVERSE;
			}
		}
		uart_rs485_rs422_init(SEG_DATA_UART0);
		//printf("UART Interface: %s mode\r\n", uart_if_mode?"RS-485":"RS-422");
	}
	
	/* Configure the UARTx */
	UART_InitStructure->UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
}


void check_uart_flow_control(uint8_t channel, uint8_t flow_ctrl)
{
	if(flow_ctrl == flow_xon_xoff)
	{
		//if((xonoff_status[channel] == UART_XON) && (M_BUFFER_USED_SIZE(channel) > UART_OFF_THRESHOLD)) // Send the transmit stop command to peer - go XOFF
		if((xonoff_status[channel] == UART_XON) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD))
        {
            //UartPutc(UART_data, UART_XOFF);
            //(channel==0)?UartPutc(UART0, UART_XOFF):UartPutc(UART1, UART_XOFF);
            (channel==0)?UartPutc(UART0, UART_XOFF):UartPutc(UART1, UART_XOFF);
			xonoff_status[channel] = UART_XOFF;
#ifdef _UART_DEBUG_
			//printf(" >> SEND XOFF [%d / %d]\r\n", M_BUFFER_USED_SIZE(channel), SEG_DATA_BUF_SIZE);
            printf(" >> SEND XOFF [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
		//else if((xonoff_status[channel] == UART_XOFF) && (M_BUFFER_USED_SIZE(channel) < UART_ON_THRESHOLD)) // Send the transmit start command to peer. -go XON
		else if((xonoff_status[channel] == UART_XOFF) && (RingBuffer_GetCount(&rxring[channel]) < UART_ON_THRESHOLD))
        {
			//UartPutc(UART0, UART_XON);
            (channel==0)?UartPutc(UART0, UART_XON):UartPutc(UART1, UART_XON);
			xonoff_status[channel] = UART_XON;
#ifdef _UART_DEBUG_
			//printf(" >> SEND XON [%d / %d]\r\n", M_BUFFER_USED_SIZE(channel), SEG_DATA_BUF_SIZE);
            printf(" >> SEND XON [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
	}
	else if(flow_ctrl == flow_rts_cts) // RTS pin control
	{
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__        
		// Buffer full occurred
		//if((rts_status[channel] == UART_RTS_LOW) && (M_BUFFER_USED_SIZE(channel) > UART_OFF_THRESHOLD))
        if((rts_status[channel] == UART_RTS_LOW) && (RingBuffer_GetCount(&rxring[channel]) > UART_OFF_THRESHOLD))
		{
			set_uart_rts_pin_high(channel);
			rts_status[channel] = UART_RTS_HIGH;
#ifdef _UART_DEBUG_
			//printf(" >> UART_RTS_HIGH [%d / %d]\r\n", M_BUFFER_USED_SIZE(channel), SEG_DATA_BUF_SIZE);
            printf(" >> UART_RTS_HIGH [%d / %d]\r\n", RingBuffer_GetCount(&rxring[channel]), SEG_DATA_BUF_SIZE);
#endif
		}
		
		// Clear the buffer full event
		//if((rts_status[channel] == UART_RTS_HIGH) && (M_BUFFER_USED_SIZE(channel) <= UART_OFF_THRESHOLD))
        if((rts_status[channel] == UART_RTS_HIGH) && (RingBuffer_GetCount(&rxring[channel]) <= UART_OFF_THRESHOLD))
		{
			set_uart_rts_pin_low(channel);
			rts_status[channel] = UART_RTS_LOW;
#ifdef _UART_DEBUG_
			printf(" >> UART_RTS_LOW [%d / %d]\r\n", M_BUFFER_USED_SIZE(channel), SEG_DATA_BUF_SIZE);
#endif
		}
#else
        ;
#endif
	}
}

int32_t uart_putc(uint8_t uartNum, uint8_t ch)
{
	//DevConfig *value = get_DevConfig_pointer();
	
	if(uartNum == SEG_DATA_UART0)
	{
		UartPutc(UART0, ch); 
	}
    else if(uartNum == SEG_DATA_UART1)
	{
		UartPutc(UART1, ch); 
	}
	else if(uartNum == SEG_DEBUG_UART)
	{
		S_UartPutc(ch);
	}
	return RET_OK;
}

int32_t uart_puts(uint8_t uartNum, uint8_t* buf, uint16_t reqSize)
{
	uint16_t lentot = 0;

	while(*buf != '\0' && lentot < reqSize)
	{
		if((uartNum == SEG_DATA_UART0)||(uartNum == SEG_DATA_UART1))
		{
			uart_putc(uartNum, *buf);
		}
		else if(uartNum == SEG_DEBUG_UART)
		{
			S_UartPutc(*buf);
		}
		else
			return 0;
		
		buf++;
		lentot++;
	}

	return lentot;
}

int32_t uart_getc(uint8_t uartNum)
{
	int32_t ch;

	if(uartNum == SEG_DATA_UART0)
	{
		/*
        while(IS_BUFFER_EMPTY(data_rx_0));
		ch = (int32_t)BUFFER_OUT(data_rx_0);
		BUFFER_OUT_MOVE(data_rx_0, 1);
        */
        while(RingBuffer_IsEmpty(&rxring[uartNum]));
		RingBuffer_Pop(&rxring[uartNum], &ch);
	}
    else if(uartNum == SEG_DATA_UART1)
	{
        /*
		while(IS_BUFFER_EMPTY(data_rx_1));
		ch = (int32_t)BUFFER_OUT(data_rx_1);
		BUFFER_OUT_MOVE(data_rx_1, 1);
        */
        while(RingBuffer_IsEmpty(&rxring[uartNum]));
		RingBuffer_Pop(&rxring[uartNum], &ch);
	}
	else if(uartNum == SEG_DEBUG_UART)
	{
		;//ch = (uint8_t)S_UartGetc();
	}
	else
		return RET_NOK;

	return ch;
}

int32_t uart_getc_nonblk(uint8_t uartNum)
{
	int32_t ch;

	if(uartNum == SEG_DATA_UART0)
	{
        /*
		if(IS_BUFFER_EMPTY(data_rx_0)) return RET_NOK;
		ch = (int32_t)BUFFER_OUT(data_rx_0);
		BUFFER_OUT_MOVE(data_rx_0,1);
        */
        if(RingBuffer_IsEmpty(&rxring[uartNum]))return RET_NOK;
		RingBuffer_Pop(&rxring[uartNum], &ch);
	}
    else if(uartNum == SEG_DATA_UART1)
	{
        /*
		if(IS_BUFFER_EMPTY(data_rx_1)) return RET_NOK;
		ch = (int32_t)BUFFER_OUT(data_rx_1);
		BUFFER_OUT_MOVE(data_rx_1,1);
        */
        if(RingBuffer_IsEmpty(&rxring[uartNum]))return RET_NOK;
		RingBuffer_Pop(&rxring[uartNum], &ch);
	}
	else if(uartNum == SEG_DEBUG_UART)
	{
		;
	}
	else
		return RET_NOK;

	return ch;
}
/*
int32_t uart_gets(uint8_t uartNum, uint8_t* buf, uint16_t reqSize)
{
	uint16_t lentot = 0, len1st = 0;

	if(uartNum == SEG_DATA_UART0)
	{
		lentot = reqSize = MIN(BUFFER_USED_SIZE(data_rx_0), reqSize);
		if(IS_BUFFER_OUT_SEPARATED(data_rx_0) && (len1st = BUFFER_OUT_1ST_SIZE(data_rx_0)) < reqSize) {
			memcpy(buf, &BUFFER_OUT(data_rx_0), len1st);
			BUFFER_OUT_MOVE(data_rx_0, len1st);
			reqSize -= len1st;
		}
		memcpy(buf+len1st, &BUFFER_OUT(data_rx_0), reqSize);
		BUFFER_OUT_MOVE(data_rx_0,reqSize);
	}
    else if(uartNum == SEG_DATA_UART1)
	{
		lentot = reqSize = MIN(BUFFER_USED_SIZE(data_rx_1), reqSize);
		if(IS_BUFFER_OUT_SEPARATED(data_rx_1) && (len1st = BUFFER_OUT_1ST_SIZE(data_rx_1)) < reqSize) {
			memcpy(buf, &BUFFER_OUT(data_rx_1), len1st);
			BUFFER_OUT_MOVE(data_rx_1, len1st);
			reqSize -= len1st;
		}
		memcpy(buf+len1st, &BUFFER_OUT(data_rx_1), reqSize);
		BUFFER_OUT_MOVE(data_rx_1,reqSize);
	}
	else if(uartNum == SEG_DEBUG_UART)
	{
		;
	}
	else
		return RET_NOK;

	return lentot;
}
*/
void uart_rx_flush(uint8_t uartNum)
{
    /*
	if(uartNum == SEG_DATA_UART0)
	{
		BUFFER_CLEAR(data_rx_0);
	}
    else if(uartNum == SEG_DATA_UART1)
	{
		BUFFER_CLEAR(data_rx_1);
	}
    */
    RingBuffer_Flush(&rxring[uartNum]);
}

uint8_t get_uart_rs485_sel(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_IN, UART0_RTS_PAD_AF); // UART0 RTS pin: GPIO / Input
		GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN); 
		
		if(GPIO_ReadInputDataBit(UART0_RTS_PORT, UART0_RTS_PIN) == UART_IF_RS422)
		{
			uart_if_mode[uartNum] = UART_IF_RS422;
		}
		else
		{
			uart_if_mode[uartNum] = UART_IF_RS485;
		}
	}
	else
	{
		GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_IN, UART1_RTS_PAD_AF); // UART1 RTS pin: GPIO / Input
		GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN); 
		
		if(GPIO_ReadInputDataBit(UART1_RTS_PORT, UART1_RTS_PIN) == UART_IF_RS422) // RS-485
		{
			uart_if_mode[uartNum] = UART_IF_RS422;
		}
		else
		{
			uart_if_mode[uartNum] = UART_IF_RS485;
		}
	}
	
	return uart_if_mode[uartNum];
}

void uart_rs485_rs422_init(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_Configuration(UART0_RTS_PORT, UART0_RTS_PIN, GPIO_Mode_OUT, UART0_RTS_PAD_AF); // UART0 RTS pin: GPIO / Output
		GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN); // UART0 RTS pin init, Set the signal low
	}
	else if(uartNum == 1) // UART1
	{
		GPIO_Configuration(UART1_RTS_PORT, UART1_RTS_PIN, GPIO_Mode_OUT, UART1_RTS_PAD_AF); // UART1 RTS pin: GPIO / Output
		GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN); // UART1 RTS pin init, Set the signal low
	}
}

void uart_rs485_enable(uint8_t uartNum)
{
	if(uart_if_mode[uartNum] == UART_IF_RS485)
	{
		// RTS pin -> High
		if(uartNum == 0) // UART0
		{
			GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN);
		}
		else if(uartNum == 1) // UART1
		{
			GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN);
		}
		delay(1);
	}
	
	//UART_IF_RS422: None
}

void uart_rs485_disable(uint8_t uartNum)
{
	if(uart_if_mode[uartNum] == UART_IF_RS485)
	{
		// RTS pin -> Low
		if(uartNum == 0) // UART0
		{
			GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN);
		}
		else if(uartNum == 1) // UART1
		{
			GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN);
		}
		delay(1);
	}
	
	//UART_IF_RS422: None
}

#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
	
uint8_t get_uart_cts_pin(uint8_t uartNum)
{
	uint8_t cts_pin;

#ifdef _UART_DEBUG_
	static uint8_t prev_cts_pin;
#endif
    
	if(uartNum == 0) // UART0
	{
		cts_pin = GPIO_ReadInputDataBit(UART0_CTS_PORT, UART0_CTS_PIN); 
	}
	else if(uartNum == 1) // UART1
	{
		cts_pin = GPIO_ReadInputDataBit(UART1_CTS_PORT, UART1_CTS_PIN); 
	}
    
#ifdef _UART_DEBUG_
	if(cts_pin != prev_cts_pin)
	{
		printf(" >> UART_CTS_%s\r\n", cts_pin?"HIGH":"LOW");
		prev_cts_pin = cts_pin;
	}
#endif
	
	return cts_pin;
}

void set_uart_rts_pin_high(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_SetBits(UART0_RTS_PORT, UART0_RTS_PIN);
	}
	else if(uartNum == 1) // UART1
	{
		GPIO_SetBits(UART1_RTS_PORT, UART1_RTS_PIN);
	}
}

void set_uart_rts_pin_low(uint8_t uartNum)
{
	if(uartNum == 0) // UART0
	{
		GPIO_ResetBits(UART0_RTS_PORT, UART0_RTS_PIN);
	}
	else if(uartNum == 1) // UART1
	{
		GPIO_ResetBits(UART1_RTS_PORT, UART1_RTS_PIN);
	}
}
#endif

