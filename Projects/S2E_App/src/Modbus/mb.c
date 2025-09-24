#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "mb.h"
#include "mbtcp.h"
#include "mbrtu.h"
#include "mbascii.h"
#include "mbserial.h"

#include "uartHandler.h"
#include "socket.h"
#include "W7500x_wztoe.h"
#include "common.h"
#include "ConfigData.h"
#include "seg.h"

extern volatile uint8_t* pucASCIIBufferCur;
extern volatile uint16_t usASCIIBufferPos;

void mbTCPtoRTU(uint8_t sock) {
    if (MBtcp2rtuFrame(sock) != false) {
        while (usRTUBufferPos) {
            UART_write((uint8_t*)pucRTUBufferCur, 1);
            pucRTUBufferCur++;
            usRTUBufferPos--;
        }
    }
}

void mbRTUtoTCP(uint8_t sock) {
    struct __network_info *net = (struct __network_info *)get_DevConfig_pointer()->network_info;

    if (MBrtu2tcpFrame() != false) {
        switch (get_device_status()) {
        case ST_UDP :
            sendto(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos, net->remote_ip, net->remote_port);
            break;
        case ST_CONNECT:
            send(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos);
            break;
        default:
            break;
        }
    }
}

void mbTCPtoASCII(uint8_t sock) {
    uint8_t ucByte;

    if (MBtcp2asciiFrame(sock) != false) {
        ucByte = MB_ASCII_START;
        UART_write(&ucByte, 1);

        while (usASCIIBufferPos) {
            ucByte = prvucMBBIN2CHAR((uint8_t) * pucASCIIBufferCur >> 4);
            UART_write(&ucByte, 1);

            ucByte = prvucMBBIN2CHAR((uint8_t) * pucASCIIBufferCur & 0x0F);
            UART_write(&ucByte, 1);

            pucASCIIBufferCur++;
            usASCIIBufferPos--;
        }
        ucByte = MB_ASCII_DEFAULT_CR;
        UART_write(&ucByte, 1);
        ucByte = MB_ASCII_DEFAULT_LF;
        UART_write(&ucByte, 1);
    }
}

void mbASCIItoTCP(uint8_t sock) {
    struct __network_info *net = (struct __network_info *)get_DevConfig_pointer()->network_info;

    if (MBascii2tcpFrame() != false) {
        switch (getSn_SR(sock)) {
        case SOCK_UDP :
            sendto(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos, net->remote_ip, net->remote_port);
            break;
        case SOCK_ESTABLISHED:
        case SOCK_CLOSE_WAIT:
            send(sock, (uint8_t*)pucTCPBufferCur, usTCPBufferPos);
            break;
        default:
            break;
        }
    }
}
