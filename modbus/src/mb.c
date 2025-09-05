#include <stdio.h>

#include "mb.h"
#include "mbtcp.h"
#include "mbrtu.h"
#include "mbserial.h"

#include "uartHandler.h"
#include "socket.h"
#include "common.h"
#include "ConfigData.h"
#include "seg.h"

extern volatile uint8_t* pucASCIIBufferCur;
extern volatile uint16_t usASCIIBufferPos;

void mbTCPtoRTU(uint8_t sock)
{
	//printf("mbTCPtoRTU\n");
  if(MBtcp2rtuFrame(sock) != FALSE)
	{
		while(usRTUBufferPos)
		{
			UART_write((uint8_t*)pucRTUBufferCur, 1);
			pucRTUBufferCur++;
			usRTUBufferPos--;
		}
	}
}

void mbRTUtoTCP(uint8_t sock)
{
	struct __network_info *network_info = (struct __network_info *)&(get_DevConfig_pointer()->network_info);

	//printf("mbRTUtoTCP\n");
	if(MBrtu2tcpFrame() != FALSE)
	{
		switch(get_device_status()) {
			case ST_UDP :
				sendto(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos, network_info->remote_ip, network_info->remote_port);
				break;
			case ST_CONNECT:
				send(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos);
				break;
			default:
				break;
		}
	}
}


