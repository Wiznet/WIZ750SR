/*******************************************************************************************************************************************************
 * Copyright ��I 2016 <WIZnet Co.,Ltd.> 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ����Software����), 
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED ����AS IS����, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************************************************************************************************/
/**
 @file		httpServer.h
 @brief 	Define constants and functions related HTTP Web server.
 */

#include <stdint.h>
#include "W7500x_wztoe.h"

#ifndef	__HTTPSERVER_H__
#define	__HTTPSERVER_H__

// HTTP Server debug message enable
//#define _HTTPSERVER_DEBUG_

#define INITIAL_WEBPAGE				"index.html"
#define M_INITIAL_WEBPAGE			"m/index.html"
#define MOBILE_INITIAL_WEBPAGE		"mobile/index.html"

/* Web Server Content Storage Select */
//#define _USE_SDCARD_
#ifndef _USE_SDCARD_
//#define _USE_FLASH_
#endif

#if !defined(_USE_SDCARD_) && !defined(_USE_FLASH_)
#define _NOTUSED_STORAGE_
#endif


/* Watchdog timer */
//#define _USE_WATCHDOG_

/*********************************************
* HTTP Process states list
*********************************************/
#define STATE_HTTP_IDLE             0           /* IDLE, Waiting for data received (TCP established) */
#define STATE_HTTP_REQ_INPROC  		1           /* Received HTTP request from HTTP client */
#define STATE_HTTP_REQ_DONE    		2           /* The end of HTTP request parse */
#define STATE_HTTP_RES_INPROC  		3           /* Sending the HTTP response to HTTP client (in progress) */
#define STATE_HTTP_RES_DONE    		4           /* The end of HTTP response send (HTTP transaction ended) */

/*********************************************
* HTTP Simple Return Value
*********************************************/
#define HTTP_FAILED					0
#define HTTP_OK						1
#define HTTP_RESET					2

/*********************************************
* HTTP Content NAME length
*********************************************/
#define MAX_CONTENT_NAME_LEN		128

/*********************************************
* HTTP Timeout
*********************************************/
#define HTTP_MAX_TIMEOUT_SEC		3			// Sec.

typedef enum
{
   NONE,		///< Web storage none
   CODEFLASH,	///< Code flash memory
   SDCARD,    	///< SD card
   DATAFLASH	///< External data flash memory
}StorageType;

typedef struct _st_http_socket
{
	uint8_t			sock_status;
	uint8_t			file_name[MAX_CONTENT_NAME_LEN];
	uint32_t 		file_start;
	uint32_t 		file_len;
	uint32_t 		file_offset; // (start addr + sent size...)
	uint8_t			storage_type; // Storage type; Code flash, SDcard, Data flash ...
}st_http_socket;

// Web content structure for file in code flash memory
#define MAX_CONTENT_CALLBACK		20

typedef struct _httpServer_webContent
{
	uint8_t	content_name[20];
	uint32_t	content_len;
	uint8_t * 	content;
}httpServer_webContent;


void httpServer_init(uint8_t * tx_buf, uint8_t * rx_buf, uint8_t cnt, uint8_t * socklist);
void reg_httpServer_cbfunc(void(*mcu_reset)(void), void(*wdt_reset)(void));
void httpServer_run(uint8_t seqnum);

void reg_httpServer_webContent(uint8_t * content_name, uint8_t * content);
uint8_t find_userReg_webContent(uint8_t * content_name, uint16_t * content_num, uint32_t * file_len);
uint16_t read_userReg_webContent(uint16_t content_num, uint8_t * buf, uint32_t offset, uint16_t size);
uint8_t display_reg_webContent_list(void);

/*
 * @brief HTTP Server 1sec Tick Timer handler
 * @note SHOULD BE register to your system 1s Tick timer handler
 */
void httpServer_time_handler(void);
uint32_t get_httpServer_timecount(void);

#endif
