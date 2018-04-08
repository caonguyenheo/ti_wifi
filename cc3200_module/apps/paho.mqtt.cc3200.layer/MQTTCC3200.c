/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stdint.h>
#include "MQTTCC3200.h"
// Free-RTOS includes
#include "FreeRTOS.h"
#include "task.h"

#include "common.h"
#include "uart_if.h"

// RTC
#include "cc_types.h"
#include "rtc_hal.h"

#include "system.h"
#include "server_http_client.h"

#define TICKS_TO_MILLIS(ticks) ((ticks) * (portTICK_PERIOD_MS))
// unsigned long MilliTimer;

static uint32_t getTimeInMillis(void)
{
#if 0	
	return TICKS_TO_MILLIS(xTaskGetTickCount());
#else
	int32_t ret_val = 0;
	struct u64_time current;
	ret_val = cc_rtc_get(&current);
	if (ret_val != 0) {
		return 0;
	}
	// UART_PRINT("current_ms=%d\r\n", (uint32_t )(current.secs *1000 + current.nsec/1000000));

	return (uint32_t )(current.secs *1000 + current.nsec/1000000);
#endif
	// MilliTimer++;
}

// static uint32_t getTimeInMillis(void)
// {
//     return ((Clock_getTicks() * Clock_tickPeriod) / 1000);
// }

char expired(Timer *timer)
{
    return ((timer->end_time > 0)
             && ((getTimeInMillis() + timer->offset) > timer->end_time));
}

void countdown_ms(Timer *timer, unsigned int timeout)
{
    uint32_t timems = getTimeInMillis();

    timer->end_time = timems + timeout;
if (timer->end_time < timems) {
        timer->offset = ~0 - timems + 1;
        timer->end_time += timer->offset;
    }
    else {
        timer->offset = 0;
    }
    // UART_PRINT("set timeout=%d, end_time=%d\r\n", timeout, timer->end_time);
}
void countdown(Timer *timer, unsigned int timeout)
{
    uint32_t timems = getTimeInMillis();

    timer->end_time = timems + (timeout * 1000);

    if (timer->end_time < timems) {
        timer->offset = ~0 - timems + 1;
        timer->end_time += timer->offset;
    }
    else {
        timer->offset = 0;
    }
}

int left_ms(Timer *timer)
{
    int diff = timer->end_time - (getTimeInMillis() + timer->offset);
    return (diff > 0 ? diff : 0);
}

void InitTimer(Timer *timer)
{
    timer->end_time = 0;
    timer->offset = 0;
}

// char expired(Timer* timer) {
// 	long left = timer->end_time - MilliTimer;
// 	return (left < 0);
// }


// void countdown_ms(Timer* timer, unsigned int timeout) {
// 	timer->end_time = MilliTimer + timeout;
// }


// void countdown(Timer* timer, unsigned int timeout) {
// 	timer->end_time = MilliTimer + (timeout * 1000);
// }


// int left_ms(Timer* timer) {
// 	long left = timer->end_time - MilliTimer;
// 	return (left < 0) ? 0 : left;
// }


// void InitTimer(Timer* timer) {
// 	timer->end_time = 0;
// }

extern HTTPCli_Struct mqttsClient;
int cc3200_read(Network* n, unsigned char* buffer, int len, int timeout_ms) {
#ifndef SERVER_MQTTS
	SlTimeval_t timeVal;
	SlFdSet_t fdset;
	int rc = 0;
	int recvLen = 0;

	SL_FD_ZERO(&fdset);
	SL_FD_SET(n->my_socket, &fdset);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = timeout_ms * 1000;
	if (sl_Select(n->my_socket + 1, &fdset, NULL, NULL, &timeVal) == 1) {
		if (SL_FD_ISSET(n->my_socket, &fdset) == 1) {
			do {
				rc = sl_Recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
				if (rc == 0) {
					break;
				}
				// Report("recv=%d, sock=%d\r\n", rc, n->my_socket);
				recvLen += rc;
			} while(recvLen < len);
		}
	}
	//Report("len %d, timeout %d, recv=%d, val=%d\r\n", len, timeout_ms, rc, buffer[0]);
	return recvLen;
#else
    int rc = 0;
    int recvLen = 0;
    int i=0;
    struct timeval tv;
    
    if (timeout_ms==0)
        timeout_ms=1;
    tv.tv_sec = timeout_ms/1000;
    tv.tv_usec = (timeout_ms%1000)*1000;
    
    if (setsockopt(mqttsClient.ssock.s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0){
        //Report("setsock error\r\n");           
    }
    //Report("mqtts read %d %d,  ", len, timeout_ms);
    do {
        rc=Ssock_recv(&mqttsClient.ssock, buffer+recvLen, len-recvLen, 0);
        if (rc == Ssock_TIMEOUT)
            rc = 0 ;
        if (rc == 0) {
            break;
        }
        recvLen += rc;
    } while(recvLen < len);

    //Report("return %d %d\r\n", recvLen, buffer[0]);
    return recvLen;
#endif
}


int cc3200_write(Network* n, unsigned char* buffer, int len, int timeout_ms) {
#ifndef SERVER_MQTTS
	SlTimeval_t timeVal;
	SlFdSet_t fdset;
	int rc = 0;
	int readySock;
/*
	SL_FD_ZERO(&fdset);
	SL_FD_SET(n->my_socket, &fdset);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = timeout_ms * 1000;
	do {
		readySock = sl_Select(n->my_socket + 1, NULL, &fdset, NULL, &timeVal);
	} while(readySock != 1);
	*/
	rc = sl_Send(n->my_socket, buffer, len, 0);
	return rc;
#else
    int ret;
    ret = HTTPCli_sendRequestBody(&mqttsClient, buffer, len);
    Report("mqtts write %d\r\n", ret);
    return ret;
#endif
}


void cc3200_disconnect(Network* n) {
	sl_Close(n->my_socket);
}


void NewNetwork(Network* n) {
	n->my_socket = 0;
	n->mqttread = cc3200_read;
	n->mqttwrite = cc3200_write;
	n->disconnect = cc3200_disconnect;
}
/*
int TLSConnectNetwork(Network *n, char* addr, int port, char* certificates, unsigned char sec_method, unsigned int cipher, char server_verify) {
	SlSockAddrIn_t sAddr;
	int addrSize;
	int retVal;
	unsigned long ipAddress;

	retVal = sl_NetAppDnsGetHostByName(addr, strlen(addr), &ipAddress, AF_INET);
	if (retVal < 0) {
		return -1;
	}

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = sl_Htons((unsigned short)port);
	sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

	addrSize = sizeof(SlSockAddrIn_t);

	n->my_socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
	if (n->my_socket < 0) {
		return -1;
	}

	SlSockSecureMethod method;
	method.secureMethod = sec_method;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method));
	if (retVal < 0) {
		return retVal;
	}

	SlSockSecureMask mask;
	mask.secureMask = cipher;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &mask, sizeof(mask));
	if (retVal < 0) {
		return retVal;
	}

	if (certificates != NULL) {
		retVal = sl_SetSockOpt(n->my_socket,
							   SL_SOL_SOCKET,
							   SL_SO_SECURE_FILES_CA_FILE_NAME,
							   certificates,
							   strlen(certificates));
		if(retVal < 0) {
			return retVal;
		}
	}

	retVal = sl_Connect(n->my_socket, (SlSockAddr_t *)&sAddr, addrSize);
	if( retVal < 0 ) {
		if (server_verify || retVal != SL_ESECSNOVERIFY) {
			sl_Close(n->my_socket);
			return retVal;
		}
	}

	// SysTickIntRegister(SysTickIntHandler);
	// SysTickPeriodSet(80000);
	// SysTickEnable();

	return retVal;
}*/

int ConnectNetwork(Network* n, char* addr, int port)
{
	SlSockAddrIn_t sAddr;
	int addrSize;
	int retVal;
	unsigned long ipAddress;

	sl_NetAppDnsGetHostByName(addr, strlen(addr), &ipAddress, AF_INET);

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = sl_Htons((unsigned short)port);
	sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

	addrSize = sizeof(SlSockAddrIn_t);

	n->my_socket = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if( n->my_socket < 0 ) {
		// error
		return -1;
	}

	retVal = sl_Connect(n->my_socket, ( SlSockAddr_t *)&sAddr, addrSize);
	if( retVal < 0 ) {
		// error
		sl_Close(n->my_socket);
	    return retVal;
	}

	// SysTickIntRegister(SysTickIntHandler);
	// SysTickPeriodSet(80000);
	// SysTickEnable();

	return retVal;
}
