#ifndef _MB_H_
#define _MB_H_

#include <stdint.h>
#include "common.h"

void mbTCPtoRTU(uint8_t sock);
void mbRTUtoTCP(uint8_t sock);

#endif

