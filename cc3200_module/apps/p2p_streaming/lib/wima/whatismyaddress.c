/*
* Copyright (C) 2014 XXX Pte. Ltd.
*
* This unpublished material is proprietary to myCin.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission
* of XXX.
*/

#include <stdio.h>
#include <stdlib.h>
/*#include <netdb.h>*/
#include "osi.h"
#include <string.h>
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"

#include "whatismyaddress.h"
#include "simplelink.h"
// #include "wima_cc3200.h"

#ifndef h_addr
#define h_addr h_addr_list[0] /* for backward compatibility */
#endif

#define DEFAULT_BINARY_SERVER_IP		0x36d039c2
/*
 * PURPOSE : Get IP address if host provided by DNS name
 * INPUT   : [address] - DNS name
 * OUTPUT  : [ip]      - Output IP
 * RETURN  : IP address
 * DESCRIPT: None
 */
static int16_t
DNSNameToIP(char* address, struct in_addr* ip)
    {
    if(address == NULL || ip == NULL)
        return -1;
    return gethostbyname(address, strlen(address), (uint32_t *)&ip->s_addr, AF_INET);
    }

static int32_t wima_nonos1(int socket, char* server_ip, int server_port, char* client_ip, int* client_port)
	{
	struct sockaddr_in dest_addr;
	struct in_addr ip;
	char res_msg[100] = {0};
	char client_ip_l[32]= {0};
	char *ptr1=NULL,*ptr2=NULL;
	unsigned int a1,a2,a3,a4=0;
	int i=0;
	ssize_t recv_len=-1;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
    
	if(DNSNameToIP(server_ip, &ip) != 0) {
		ip.s_addr = DEFAULT_BINARY_SERVER_IP;
	}
	
	memset((char *)&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = htonl(ip.s_addr);
	dest_addr.sin_port = htons(server_port);
	
	// make message
	char req_msg[100]="{\"request\":\"get\",\"id\":2,\"uri\":\"mypublicaddress\"}";
	
	// then send
	ssize_t sent = sendto(socket, req_msg, strlen(req_msg),0,(struct sockaddr*)&dest_addr, sizeof(dest_addr));
	
	if (sent < 0)
	{
		UART_PRINT("can not send udp message\n");
		return -1;
	}

	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv))<0)
		UART_PRINT("Set udp socket timeout error\n");

    // read now
    for(i=0;i<40;i++){
		recv_len = recv(socket, res_msg, sizeof(res_msg), 0);
		if (recv_len>0)
			break;
		if (i==0)
			sendto(socket, req_msg, strlen(req_msg),0,(struct sockaddr*)&dest_addr, sizeof(dest_addr));
	}
	if (i==40)
		return -1;
	// UART_PRINT("%s\n",res_msg);
	
	// get inv(IP)
	ptr1 = strstr(res_msg, "\"ip\"");
	ptr1 +=4;
	ptr1 = strstr(ptr1, "\"");
	ptr1++;
	if(ptr1 == NULL) {
		return -1;
	}
	ptr2=ptr1+1;
	ptr2 = strstr(ptr2, "\"");
	memcpy(client_ip_l,ptr1,ptr2-ptr1);
	// convert inv(IP) to IP
	//UART_PRINT("%s\n",client_ip_l);
	ptr2 = strstr(client_ip_l, ".");
	*ptr2=0;
	a1=(atoi(client_ip_l)&0xff);
	ptr2++;
	ptr1 = strstr(ptr2, ".");
	*ptr1=0;
	a2=(atoi(ptr2)&0xff);
	ptr1++;
	ptr2 = strstr(ptr1, ".");
	*ptr2=0;
	a3=(atoi(ptr1)&0xff);
	ptr2++;
	a4=(atoi(ptr2)&0xff);
	
	for(i=0;i<32;i++)
		client_ip_l[0]=0;
	snprintf(client_ip_l,32,"%u.%u.%u.%u",a1^0xff,a2^0xff,a3^0xff,a4^0xff);
	for(i=0;i<32;i++){
		client_ip[i]=client_ip_l[i];
		if (client_ip_l[i]==0)
			break;
	}
	// get inv(port)
	ptr1 = strstr(res_msg, "\"port\"");
	ptr1++;
	ptr1 = strstr(ptr1, ":");
	ptr1++;
	ptr2=ptr1;
	while((*ptr2>='0') && (*ptr2<='9'))
		ptr2++;
	*ptr2=0;
	// return port
	*client_port = (atoi(ptr1)^0xffff)&0xffff;
	
	// Flush data
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
	recv(socket, res_msg, sizeof(res_msg), 0);
	return 0;
	}


int32_t wima_nonos(int socket, char* server_ip, int server_port, char* client_ip, int* client_port){
	if (wima_nonos1(socket,server_ip, server_port, client_ip, client_port)!=0)
		return wima_nonos1(socket,server_ip, server_port, client_ip, client_port);
	else
		return 0;
}
