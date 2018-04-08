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

#include "timer_interface.h"
#include "network_interface.h"
#include "aws_iot_error.h"

#define CHECK(x) do {   \
    if ((x) < 0)        \
        return (x);     \
} while(0)

unsigned long MilliTimer;

void SysTickIntHandler(void)
{
    MilliTimer++;
}

char expired(Timer* timer)
{
    long left = timer->end_time - MilliTimer;
    return (left < 0);
}


void countdown_ms(Timer* timer, unsigned int timeout)
{
    timer->end_time = MilliTimer + timeout;
}


void countdown(Timer* timer, unsigned int timeout)
{
    timer->end_time = MilliTimer + (timeout * 1000);
}


int left_ms(Timer* timer)
{
    long left = timer->end_time - MilliTimer;
    return (left < 0) ? 0 : left;
}


void InitTimer(Timer* timer)
{
    timer->end_time = 0;
}


int iot_tls_init(Network* n)
{
    if (n == NULL) {
        return (NULL_VALUE_ERROR);
    }

    n->my_socket = 0;
    n->mqttread = iot_tls_read;
    n->mqttwrite = iot_tls_write;
    n->disconnect = iot_tls_disconnect;

    return (NONE_ERROR);
}

int iot_tls_connect(Network *n, TLSConnectParams TLSParams)
{

    SlSockAddrIn_t sAddr;
    int addrSize;
    unsigned long ipAddress;

    if (n == NULL) {
        return (NULL_VALUE_ERROR);
    }

    // get ip address
    CHECK(sl_NetAppDnsGetHostByName(TLSParams.pDestinationURL,
        strlen(TLSParams.pDestinationURL),
        &ipAddress,
        AF_INET));

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = sl_Htons(TLSParams.DestinationPort);
    sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

    addrSize = sizeof(SlSockAddrIn_t);

    // create socket
    n->my_socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
    CHECK(n->my_socket);

    // make it secure
    SlSockSecureMethod method;
    method.secureMethod = SL_SO_SEC_METHOD_TLSV1_2;
    CHECK(sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method)));

    SlSockSecureMask mask;
    mask.secureMask = SL_SEC_MASK_SECURE_DEFAULT;
    CHECK(sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &mask, sizeof(mask)));

    //configure the socket with certificate - for server verification
    CHECK(sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
                           TLSParams.pRootCALocation, \
                           strlen(TLSParams.pRootCALocation)));

    // private key
    CHECK(sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, \
                            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,\
                            TLSParams.pDevicePrivateKeyLocation, \
                            strlen(TLSParams.pDevicePrivateKeyLocation)));
    // private cer
    CHECK(sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, \
                         SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, \
                         TLSParams.pDeviceCertLocation, \
                         strlen(TLSParams.pDeviceCertLocation)));
    // then connect
    CHECK(sl_Connect(n->my_socket, ( SlSockAddr_t *)&sAddr, addrSize));

    SysTickIntRegister(SysTickIntHandler);
    SysTickPeriodSet(80000);
    SysTickEnable();

    return NONE_ERROR;
}

int iot_tls_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{

    SlTimeval_t timeVal;
    SlFdSet_t fdset;
    int rc = 0;
    int readySock;

    if (n == NULL || buffer == NULL || n->my_socket == 0 || timeout_ms == 0) {
        return (NULL_VALUE_ERROR);
    }

    SL_FD_ZERO(&fdset);
    SL_FD_SET(n->my_socket, &fdset);

    timeVal.tv_sec = 0;
    timeVal.tv_usec = timeout_ms * 1000;

    do {
        readySock = sl_Select(n->my_socket + 1, NULL, &fdset, NULL, &timeVal);
    } while (readySock != 1);

    rc = sl_Send(n->my_socket, buffer, len, 0);

    return rc;
}


int iot_tls_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{

    SlTimeval_t timeVal;
    SlFdSet_t fdset;
    int rc = 0;
    int recvLen = 0;

    if (n == NULL || buffer == NULL || n->my_socket == 0 || timeout_ms == 0) {
        return (NULL_VALUE_ERROR);
    }

    SL_FD_ZERO(&fdset);
    SL_FD_SET(n->my_socket, &fdset);

    timeVal.tv_sec = 0;
    timeVal.tv_usec = timeout_ms * 1000;
    if (sl_Select(n->my_socket + 1, &fdset, NULL, NULL, &timeVal) == 1) {
        do {
            rc = sl_Recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
            recvLen += rc;
        } while(recvLen < len);
    }
    return recvLen;
}


void iot_tls_disconnect(Network* n)
{
    if (n == NULL || n->my_socket == 0) {
        return;
    }
    sl_Close(n->my_socket);
    SysTickDisable();
}

int iot_tls_destroy(Network *n)
{
    if (n == NULL) {
        return (NULL_VALUE_ERROR);
    }

    n->my_socket = 0;
    n->mqttread = NULL;
    n->mqttwrite = NULL;
    n->disconnect = NULL;

    return (NONE_ERROR);
}
