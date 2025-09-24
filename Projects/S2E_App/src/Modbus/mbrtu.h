#include <stdbool.h>

#ifndef _MBRTU_H
#define _MBRTU_H

#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     DATA_BUF_SIZE - 7    /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

#define MB_RTU_ADDR_SIZE				1

extern volatile uint8_t *pucTCPBufferCur;
extern volatile uint16_t usTCPBufferPos;

bool MBrtu2tcpFrame(void);
void eMBRTUInit(uint32_t ulBaudRate);
void RTU_Uart_RX(void);
#endif


