//****************************************************************************
//
//! \addtogroup p2p
//! @{
//
//****************************************************************************
#include <string.h>
#include <stdio.h>
#include "p2p.h"
#include "simplelink.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "cc3200_aes.h"
#include "date_time.h"
#include "inet_pton.h"
#include "osi.h"
#include "network_if.h"
#include "cc3200_system.h"
#include "whatismyaddress.h"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#define STR_IP_LEN				32
// PL
static int32_t pl_sock_fd = -1;
static char str_pl_ip[STR_IP_LEN] = {0};
static uint32_t pl_port = 0;

// PR
static int32_t pr_sock_fd = -1;
static char str_pr_ip[STR_IP_LEN] = {0};
static uint32_t pr_port = 0;

extern char ps_ip[];
extern uint32_t ps_port;
extern char pr_ip[];
extern uint32_t pr_port;
extern char p2p_key_char[];
extern char p2p_rn_char[];
char SERVER_URL[40];
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************
//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static inline int32_t get_local_ip(char *local_ip);

//*****************************************************************************
//                      PUBLIC FUNCTION
//*****************************************************************************
int get_pl_socket(char *my_pl_ip, uint32_t *my_pl_port) {
	int ret_val = 0;

	if (my_pl_ip == NULL || my_pl_port == NULL) {
		return -1;
	}

	// create new one if not exist
	if (pl_sock_fd < 0 ) {
	    ret_val = new_pl_socket(str_pl_ip, &pl_port);
	    if (ret_val < 0) {
	    	// Return error code
	    	return ret_val;
	    }
	   	// save socket fd
	   	pl_sock_fd = ret_val;
	}

	// retrive old one
	strcpy(my_pl_ip, str_pl_ip);
	*my_pl_port = pl_port;
	return pl_sock_fd;
}

int get_pr_socket(char *my_pr_ip, uint32_t *my_pr_port) {
	int ret_val = 0;

	if (my_pr_ip == NULL || my_pr_port == NULL) {
		return -1;
	}

	// create new one if not exist
	if (pr_sock_fd < 0) {
		ret_val = new_pr_socket(str_pr_ip, &pr_port);
		if (ret_val < 0) {
			// Return error code
			return ret_val;
		}
		pr_sock_fd = ret_val;
	}

	// Retrive sock fd
	strcpy(my_pr_ip, str_pr_ip);
	*my_pr_port = pr_port;
	return pr_sock_fd;
}
 
//===============================================================================//
//   			PL MODE                                                          //
//===============================================================================//
//*****************************************************************************
//
//! create new local socket
//!
//! \brief  create local  socket for p2p streaming
//!
//! \param[out]  my_pl_ip: ip address of device in string format
//! \param[out]  my_pl_port: port of local udp socket
//!
//! \return 0 : success, -ve : failure
//!
//
//*****************************************************************************
int new_pl_socket(char *my_pl_ip, uint32_t *my_pl_port) {
	int32_t sock_id;
	int32_t ret_val;
	struct sockaddr_in src_addr;
	// Pre-condition
	if (my_pl_ip == NULL || my_pl_port == NULL) {
		return -1;
	}
	// Create socket
	sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_id < 0) {
		UART_PRINT("cannot create socket\n");
		return sock_id;
	}
	memset(&src_addr, 0, sizeof(struct sockaddr_in));
	src_addr.sin_family = AF_INET;
	src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	src_addr.sin_port = htons(PL_PORT);

	ret_val = bind(sock_id, (struct sockaddr *)&src_addr, sizeof(struct sockaddr));
	if (ret_val != 0) {
		UART_PRINT("cannot binding socket\n");
		goto exit_error;
	}
	// Get local IP
	// Get socket port
	// Post-condition
	*my_pl_port = ntohs(src_addr.sin_port);
	ret_val = get_local_ip(my_pl_ip);
	if (ret_val != 0) {
		UART_PRINT("cannot get ip\n");
		goto exit_error;
	}
exit_ok:
	return sock_id;
exit_error:
	close(sock_id);
	return ret_val;
}

//===============================================================================//
//   			PR MODE                                                          //
//===============================================================================//
//*****************************************************************************
//
//! create new remote socket
//!
//! \brief  create remote socket for p2p streaming
//!
//! \param[out]  my_pr_ip: ip address of device in string format
//! \param[out]  my_pr_port: port of local udp socket
//!
//! \return 0 : success, -ve : failure
//!
//
//*****************************************************************************
extern int32_t pr_socket;
int new_pr_socket(char *my_pr_ip, uint32_t *my_pr_port) {
    int ret_val = 0;
    int client_sock;
    char server_url[32] = {0}, client_ip[32] = {0};
    int  serverPort = SERVER_PORT, client_port=0;
	struct sockaddr_in src_addr;
	
	UART_PRINT("\r\nStart PR open, current sock: %d",pr_socket);
	
    // Pre-condition
    if (my_pr_ip == NULL || my_pr_port == NULL) {
        return -1;
    }
    //memset(client_ip, 0, sizeof(client_ip));
    //memset(server_url, 0, sizeof(server_url));

    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        UART_PRINT("cannot create socket\n");
        return client_sock;
    }

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.sin_family 		= AF_INET;
	src_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
	src_addr.sin_port 			= htons(PR_PORT);

	ret_val = bind(client_sock, (struct sockaddr *)&src_addr, sizeof(src_addr));
	if (ret_val != 0) {
		UART_PRINT("cannot binding PR socket, ret = %d\r\n", ret_val);
		close(client_sock);
		return ret_val;
	}
	
    strcpy(server_url, SERVER_URL);
    UART_PRINT("server_url %s",server_url);
    // Note, socket timeout is set to 50ms internally
    ret_val = wima_nonos(client_sock, server_url, serverPort, client_ip, &client_port);
    if ( ret_val < 0) {
        UART_PRINT("cannot query for my public address\n");
        close(client_sock);
        return ret_val;
    }

    UART_PRINT("\r\nip:%s, port: %d", client_ip, client_port);

    // Post-condition
    strcpy(my_pr_ip, client_ip);
    *my_pr_port = client_port;
    return client_sock;
}

//*****************************************************************************
//                      PRIVATE FUNCTION
//*****************************************************************************
static inline int32_t get_local_ip(char *local_ip) {
	uint32_t ul_IP = 0;
    uint32_t ul_SubMask = 0;
    uint32_t ul_DefGateway = 0;
    uint32_t ul_Dns = 0;
	int32_t ret_val;

    if (local_ip == NULL) {
    	return -1;
    }

	ret_val = Network_IF_IpConfigGet(&ul_IP, &ul_SubMask, &ul_DefGateway, &ul_Dns);
	ASSERT_ON_ERROR(ret_val);

	sprintf(local_ip, "%d.%d.%d.%d",
		SL_IPV4_BYTE(ul_IP, 3), SL_IPV4_BYTE(ul_IP, 2),
		SL_IPV4_BYTE(ul_IP, 1), SL_IPV4_BYTE(ul_IP, 0)
		);
	return ret_val;
}

void PS_open(int ps_socket_l){
	int i;
	char l_packet[34]={'K','H','O','I',0x00,32};
	struct sockaddr_in dest_addr;
	struct in_addr l_ip;
	_u32 uiDestIp;
    long lRetVal = -1;
    char device_mac[MAX_MAC_ADDR_SIZE] = {0};
    uint32_t current_time_s = dt_get_time_s();
    UART_PRINT("currnet_time_s = %d\r\n", current_time_s);
    get_mac_address(device_mac);
    
	for (i=0;i<12;i++)
		l_packet[i+6]=device_mac[i];
	for (i=0;i<12;i++)
		l_packet[i+6+12]=p2p_rn_char[i];
	l_packet[12+6+12+0]=(current_time_s>>24)&0xff;
	l_packet[12+6+12+1]=(current_time_s>>16)&0xff;
	l_packet[12+6+12+2]=(current_time_s>>8)&0xff;
	l_packet[12+6+12+3]=(current_time_s>>0)&0xff;
	aes_encrypt_ecb(p2p_key_char, l_packet+18);
	
	
	    
    // /* Resolve HOST NAME/IP */
    // lRetVal = sl_NetAppDnsGetHostByName((signed char *)ps_ip,
    //                                       strlen((const char *)ps_ip),
    //                                       &uiDestIp,SL_AF_INET);
                                         
    lRetVal = inet_pton4(ps_ip, (unsigned char *)&uiDestIp);
    // return 0 means invalid
    if (lRetVal == 0) {
        UART_PRINT("PS Open: INVALID IP\n");
    }
                 
	memset((char *)&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = uiDestIp;
	dest_addr.sin_port = htons(ps_port);
	
	ssize_t sent = sendto(ps_socket_l, l_packet, 34,0,(struct sockaddr*)&dest_addr, sizeof(dest_addr));
	sent = sendto(ps_socket_l, l_packet, 34,0,(struct sockaddr*)&dest_addr, sizeof(dest_addr));
	/*l_packet[0]='K';
	l_packet[1]='H';
	l_packet[2]='O';
	l_packet[3]='I';
	sent = sendto(ps_socket_l, l_packet, 34,0,(struct sockaddr*)&dest_addr, sizeof(dest_addr));*/
	if (sent < 0)
	{
		UART_PRINT("can not send udp message\n");
	}
	UART_PRINT("\r\nSend PS Open to server done \r\n");
}

int new_ps_socket(void) {
	int32_t sock_id;
	int32_t ret_val;
	struct sockaddr_in src_addr;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	
	sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_id < 0) {
		UART_PRINT("cannot create socket\n");
		return sock_id;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.sin_family 		= AF_INET;
	src_addr.sin_addr.s_addr 	= htonl(INADDR_ANY);
	src_addr.sin_port 			= htons(PS_PORT);

	ret_val = bind(sock_id, (struct sockaddr *)&src_addr, sizeof(src_addr));
	if (ret_val != 0) {
		UART_PRINT("cannot binding socket\n");
		close(sock_id);
		return ret_val;
	}
	ret_val = setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
	if (ret_val<0){
		UART_PRINT("Set udp socket timeout error\n");
		close(sock_id);
		return ret_val;
	}
	return sock_id;
}

void PR_open(int l_pr_socket)
{
	char l_packet[1000 + 6] = {'V','L','V','L',0x00, 0x00};
	struct sockaddr_in dest_addr;
	_u32 uiDestIp;
    long lRetVal = -1;
    ssize_t sent = -1;
    int i;
    // feed data for send buffer
    for (i = 0; i < 1000; i++) {
    	l_packet[i+6] = i%100;
    };
    lRetVal = inet_pton4(pr_ip, (unsigned char *)&uiDestIp);
    // return 0 means invalid
    if (lRetVal == 0) {
        UART_PRINT("PR: INVALID IP\n");
    }
	memset((char *)&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = uiDestIp;
	dest_addr.sin_port = htons(pr_port);

	// send pr open packet
	for (i = 0; i < 2; i++) {
		sent = sendto(l_pr_socket, l_packet, 100 + 6, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
		osi_Sleep(10);

		sent = sendto(l_pr_socket, l_packet, 500 + 6, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
		osi_Sleep(10);

		sent = sendto(l_pr_socket, l_packet, 1000 + 6, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
		if (sent < 0)
		{
			UART_PRINT("can not send PR message\n");
		}
		osi_Sleep(10);
	}

}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
