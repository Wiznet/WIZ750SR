#ifndef _MB_H_
#define _MB_H_

#define MODBUS_NONE        0
#define MODBUS_RTU         1
#define MODBUS_ASCII       2

void mbTCPtoRTU(uint8_t sock);
void mbRTUtoTCP(uint8_t sock);
void mbASCIItoTCP(uint8_t sock);
void mbTCPtoASCII(uint8_t sock);
#endif

