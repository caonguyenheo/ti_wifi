#include <stdlib.h>
#include <stdio.h>
#include "gpio_if.h"
#include "user_common.h"
// Driverlib Includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "ringbuffer.h"
#include "ip_over_spi.h"
#include "spi_slave_handler.h"

#define UINIT_TEST_IOS				(0)

#define SOCKET_TIMEOUT_S  			(0)
#define SOCKET_TIMEOUT_US			(5000)
extern RingBuffer* ring_buffer_send; 
static int ios_free_transfer_data(IPOVERSPI_TRANSFER *t_data)
{
	memset((char *)t_data, 0, sizeof(IPOVERSPI_TRANSFER));
}
static int socket_open(char type, const char *host, int port)
{
	int rt;
	switch(type)
	{
		case IOS_TYPE_TCP:
			rt = tcp_open(host, port);
		break;
		case IOS_TYPE_UDP:
			rt = udp_open(host, port);
		break;
		default:
			rt = -2;
		break;
	}
	return rt;
}
// static int socket_close(char type, int socket_id)
// {
// 	int rt;
// 	switch(type)
// 	{
// 		case IOS_TYPE_TCP:
// 		case IOS_TYPE_UDP:
// 			rt = close(socket_id);
// 		break;
// 		default:
// 			rt = -2;
// 		break;
// 	}
// 	return rt;
// }
static int socket_write(char type, int socket_id, char *buffer, int length)
{
	int rt;
	switch(type)
	{
		case IOS_TYPE_TCP:
			rt = tcp_write(socket_id, buffer, length);
		break;
		case IOS_TYPE_UDP:
			rt = udp_write(socket_id, buffer, length);
		break;
		default:
			rt = -2;
		break;
	}
	return rt;
}
static int socket_read(char type, int socket_id, char *buffer, int length)
{
	int rt;
	switch(type)
	{
		case IOS_TYPE_TCP:
		case IOS_TYPE_UDP:
			// Cause it's blocking socket with timeout so tcp/udp read are the same
			rt = tcp_read(socket_id, buffer, length);
		break;
		// case IOS_TYPE_UDP:
			// rt = udp_read(socket_id, buffer, length);
		// break;
		default:
			rt = -2;
		break;
	}
	return rt;
}
int ios_handler(char packet_type, char *in_data, int in_len)
{
	int i, len, rt = -1;
	char *tmp;
	char type, id, control, err_code, direction, fragment;
	char *real_data;
	IPOVERSPI_TRANSFER t_response;
	ios_packet_parse((IPOVERSPI_TRANSFER*)in_data, &type, &id, &control, &err_code, &direction, &len, &fragment, &real_data);	
	UART_PRINT("ios_handler %x, %x, %x, %x, %d, %x\n\r", type, id, control, direction, len, fragment);
	switch(type)
	{
		case IOS_TYPE_TCP:
		case IOS_TYPE_UDP:
			switch(control)
			{
				case IOS_CONTROL_OPEN:
				{
					char *tmp;
					char host[64] = {0};
					int port;
					tmp = strstr(real_data, ":");
					if(tmp)
					{
						memcpy(host, real_data, tmp-real_data);
						port = atoi(tmp+1);
						rt = socket_open(type, host, port);
						//RingBuffer_Write(ring_buffer_send, TCP_OPEN_RESPONSE, 1024);
					}else
					{
			//ios_packet_create(IPOVERSPI_TRANSFER * data, char type, char id, char control, char err_code, char direction, int len, char fragment, char *buffer)
						rt = -2;
						UART_PRINT("ios_handler: IOS_CONTROL_OPEN wrong request %s\n\r", real_data);
					}
					ios_packet_create(&t_response, type, (char)rt, control, (char)rt, IOS_DIRECTION_W2H, len, IOT_FRAGMENT_LAST, real_data);
					RingBuffer_Write(ring_buffer_send, (char *)&t_response, TRANSPORT_TRANSFER_LENGTH);
					UART_PRINT("ios_handler: IOS_CONTROL_OPEN send response %d\n\r", rt);
				}
				break;
				case IOS_CONTROL_CLOSE:
				#if (UINIT_TEST_IOS)
					rt = 0;
				#else
					rt = close(id);
				#endif
					ios_packet_create(&t_response, type, id, control, (char)rt, IOS_DIRECTION_W2H, len, IOT_FRAGMENT_LAST, real_data);
					RingBuffer_Write(ring_buffer_send, (char *)&t_response, TRANSPORT_TRANSFER_LENGTH);
					UART_PRINT("ios_handler: IOS_CONTROL_CLOSE send response %d\n\r", rt);
					//UART_PRINT("SEND RESPONSE 2 %d\n\r", ring_buffer_send->end);
				break;
				case IOS_CONTROL_SEND:
					rt = socket_write(type, id, real_data, len);
					ios_packet_create(&t_response, type, id, control, (rt>0)?0:-1, IOS_DIRECTION_W2H, rt, IOT_FRAGMENT_LAST, NULL);
					RingBuffer_Write(ring_buffer_send, (char *)&t_response, TRANSPORT_TRANSFER_LENGTH);
					UART_PRINT("ios_handler: IOS_CONTROL_SEND send response %d\n\r", rt);
					//UART_PRINT("SEND RESPONSE 3 %d\n\r", ring_buffer_send->end);
				break;
				case IOS_CONTROL_RECEIVE:
				{
					int remain_len = len;
					int recv_len, is_last_packet;
					while(remain_len>0)
					{
						ios_free_transfer_data(&t_response);
						recv_len = (remain_len>TRANSPORT_DATA_LENGTH)?TRANSPORT_DATA_LENGTH:remain_len;
						rt = socket_read(type, id, t_response.transport_data, recv_len);
						if(rt>0)
							remain_len -= rt;
						UART_PRINT("ios_handler: IOS_CONTROL_RECEIVE socket_read %d, remain_len %d\n\r", rt, remain_len);
						is_last_packet = ((remain_len<=0)||(rt<=0))?IOT_FRAGMENT_LAST:IOT_FRAGMENT_CONT;
						ios_packet_create(&t_response, type, id, control, 0, IOS_DIRECTION_W2H, (rt<=0)?0:rt, is_last_packet, NULL);
						RingBuffer_Write(ring_buffer_send, (char *)&t_response, TRANSPORT_TRANSFER_LENGTH);
						UART_PRINT("ios_handler: IOS_CONTROL_RECEIVE send response %d\n\r", rt);
						if(rt<=0)
							break;

					}
				}
				break;
			}
		break;
		case IOS_TYPE_PING:
		{
			char response[1024] = {0};
			if(strlen(real_data)<=0)
			{
	//ios_packet_create(IPOVERSPI_TRANSFER * data, char type, char id, char control, char err_code, char direction, int len, char fragment, char *buffer)
				rt = -1;
				UART_PRINT("ios_handler: IOS_TYPE_PING missing hostname\n\r");
			}else
			{
				rt = ping(real_data, response, sizeof(response));
			}
			ios_packet_create(&t_response, type, id, control, (rt>0)?1:0, IOS_DIRECTION_W2H, rt, IOT_FRAGMENT_LAST, response);
			RingBuffer_Write(ring_buffer_send, (char *)&t_response, TRANSPORT_TRANSFER_LENGTH);
			UART_PRINT("ios_handler: IOS_TYPE_PING send response %d\n\r", rt);
		}
		break;
		case IOS_TYPE_GET_RESP:
			switch(control)
			{
				case IOS_CONTROL_OPEN:
					set_response_flag(1);
				break;
				case IOS_CONTROL_CLOSE:
					set_response_flag(0);
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
	return rt;
}
/*
* purpose: Open TCP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  >0: socket id
*/
int tcp_open(const char *host, int port)
{
#if (UINIT_TEST_IOS)
	UART_PRINT("tcp_open: %s:%d\n\r", host, port);
	return 1;
#else
	int32_t sock_fd = -1;
	int32_t ret_val = -1;
	uint32_t ip_addr = 0;
	struct sockaddr_in dest_addr;
	struct timeval tv;

	if (host == NULL) {
		UART_PRINT("udp_open: host is NULL\n\r");
		return -1;
	}

	UART_PRINT("tcp_open: %s:%d\n\r", host, port);

	// Create socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		ret_val = sock_fd;
		goto exit_error;
	}
	UART_PRINT("tcp_open: sockid=%d\r\n", sock_fd);

	// Set socket timeout
	memset((char *)&tv, 0x00, sizeof(tv));
	tv.tv_sec = SOCKET_TIMEOUT_S;
	tv.tv_usec = SOCKET_TIMEOUT_US;
	ret_val = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret_val < 0) {
		goto exit_sock_error;
	}

	// Get dest ip
	ret_val = gethostbyname((signed char *)host, strlen(host), &ip_addr, AF_INET);
	if (ret_val < 0) {
		goto exit_sock_error;
	}
	char *ptr_char = (char *) &ip_addr;
	UART_PRINT("tcp_open: ip = %u.%u.%u.%u\r\n", ptr_char[3]
											, ptr_char[2]
											, ptr_char[1]
											, ptr_char[0]);

	// Setup addr struct
	memset((char *)&dest_addr, 0x00, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = htonl(ip_addr);
	dest_addr.sin_port = htons(port);

	// connect to server
	ret_val = connect(sock_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (ret_val < 0) {
		UART_PRINT("tcp_open: connect failed\r\n");
		goto exit_sock_error;
	}
	UART_PRINT("tcp_open: connected host = %s\r\n", host);
	
exit_ok:
	return sock_fd;

exit_sock_error:
	close(sock_fd);

exit_error:
	ASSERT_ON_ERROR(ret_val);
#endif
}

/*
* purpose: Close TCP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  0: successful
*/
// int tcp_close(int socket_id)
// {
// 	UART_PRINT("tcp_close: %d\n\r", socket_id);
// 	return 0;
// }

/*
* purpose: Read TCP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: read length
* return: -1: error
*		  >0: real length read
*/
int tcp_read(int socket_id, char *buffer, int length)
{
#if (UINIT_TEST_IOS)
	static int count = 10;
	static int a = 0x26;
	char *tmp;
	int i;
	UART_PRINT("tcp_read: %d, %d\n\r", socket_id, length);
	if(count-- >0)
	{
		UART_PRINT("tcp_read: memcpy %x %x %d \n\r", buffer, tmp, length);
		buffer[0] = a++;
		buffer[1] = 0x30;
		buffer[2] = 0x21;
		buffer[3] = 0x12;
		buffer[1014] = 0x12;
		//memcpy(buffer, tmp, length);
		return length;	
		//UART_PRINT("SEND RESPONSE 4 %d\n\r", ring_buffer_send->end);
		//RingBuffer_Write(ring_buffer_send, TCP_READ_RESPONSE1, 1024);
		//UART_PRINT("SEND RESPONSE 5 %d\n\r", ring_buffer_send->end);
		//RingBuffer_Write(ring_buffer_send, TCP_READ_RESPONSE2, 1024);
	}else
	{
		count = 10;
		return -1;
	}
	return 0;
#else
	int recv_len = 0;
	int ret_val = 0;
	struct timeval tv;
	struct fd_set fdset;

	UART_PRINT("tcp_read: read socket_id=%d length=%d\r\n", socket_id, length);
	if (buffer == NULL) {
		UART_PRINT("tcp_read: buffer is NULL\r\n");
		return -1;
	}

	FD_ZERO(&fdset);
	FD_SET(socket_id, &fdset);

	// setup time out
	memset((char *)&tv, 0x00, sizeof(tv));
	tv.tv_sec = SOCKET_TIMEOUT_S;
	tv.tv_usec = SOCKET_TIMEOUT_US;

	if (select(socket_id +1, &fdset, NULL, NULL, &tv) == 1) {
		do {
			ret_val = recv(socket_id, buffer + recv_len, length - recv_len, 0);
			if (ret_val > 0) {
				UART_PRINT("tcp_read: read %d bytes\r\n", ret_val);
				recv_len += ret_val;
			}
			else if (ret_val == 0) {
				UART_PRINT("tcp_read: end\r\n", ret_val);
				break;
			}
			else {
				// Timeout -> break and return
				if (ret_val == EWOULDBLOCK || ret_val == EAGAIN) {
					UART_PRINT("tcp_read: read time out, read %d bytes\r\n", recv_len);
					break;
				}

				UART_PRINT("tcp_read: read %d bytes\r\n", ret_val);
				recv_len = ret_val;
				break;
			}
		} while (recv_len < length);
	}

	return recv_len;
#endif
}

/*
* purpose: write TCP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: write length
* return: -1: error
*		  >0: real length write
*/
int tcp_write(int socket_id, char *buffer, int length)
{
#if (UINIT_TEST_IOS)
	UART_PRINT("tcp_write: %d, %d\n\r", socket_id, length);
	return length;
#else
	int ret_val = -1;
	int remain_len = length;

	if (buffer == NULL) {
		UART_PRINT("tcp_write: buffer is NULL\n\r");
		return -1;
	}

	while (remain_len > 0) {
		ret_val = send(socket_id, buffer + (length - remain_len), length, 0);
		// FIXME: error handle
		if (ret_val == 0) {
			UART_PRINT("tcp_write: end\r\n");
			// Incase not send all data
			length -= remain_len;
			break;
		}
		else if (ret_val > 0) {
			UART_PRINT("tcp_write: send %d bytes\r\n", ret_val);
			remain_len -= ret_val;
		}
		else {
			UART_PRINT("tcp_write: send error %d\r\n", ret_val);
			// return error value
			remain_len = ret_val;
			break;
		}
	}
	return length;
#endif
}
 /******UDP*****/
 /*
* purpose: Open UDP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  >0: socket id
*/
int udp_open(const char *host, int port)
{
#if (UINIT_TEST_IOS)
	UART_PRINT("udp_open: %s:%d\n\r", host, port);
	return 2;
#else
	int32_t ret_val = -1;
	int32_t sock_fd = -1;
	uint32_t ip_addr = 0;
	struct sockaddr_in dest_addr;
	struct timeval tv;

	if (host == NULL) {
		UART_PRINT("udp_open: host is NULL\n\r");
		return -1;
	}

	UART_PRINT("udp_open: %s:%d\n\r", host, port);

	// create socket
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		ret_val = sock_fd;
		goto exit_error;
	}
	
	// Set socket timeout
	memset((char *)&tv, 0x00, sizeof(tv));
	tv.tv_sec = SOCKET_TIMEOUT_S;
	tv.tv_usec = SOCKET_TIMEOUT_US;
	ret_val = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret_val < 0) {
		goto exit_sock_error;
	}
	// get ip
	ret_val = gethostbyname((signed char *)host, strlen(host), &ip_addr, AF_INET);
	if (ret_val < 0) {
		UART_PRINT("udp_open: failed to get ip\r\n");
		goto exit_sock_error;
	}

	char *ptr_char = (char *) &ip_addr;
	UART_PRINT("udp_open: ip = %u.%u.%u.%u\r\n", ptr_char[3]
											, ptr_char[2]
											, ptr_char[1]
											, ptr_char[0]);

	// bind adress
	memset((char *)&dest_addr, 0x00, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = htonl(ip_addr);
	dest_addr.sin_port = htons(port);

	ret_val = connect(sock_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (ret_val < 0) {
		UART_PRINT("udp_open: cannot connect to server\r\n");
		goto exit_sock_error;
	}

exit_ok:
	return sock_fd;
exit_sock_error:
	close(sock_fd);
exit_error:
	ASSERT_ON_ERROR(ret_val);
#endif
}

/*
* purpose: Close UDP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  0: successful
*/
// int udp_close(int socket_id)
// {
// #if (UINIT_TEST_IOS)
// 	UART_PRINT("udp_close: %d\n\r", socket_id);
// 	return 0;
// #else
// #endif
// }

#if (0)
/*
* purpose: Read UDP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: read length
* return: -1: error
*		  >0: real length read
*/
int udp_read(int socket_id, char *buffer, int length)
{
#if (UINIT_TEST_IOS)
	static int count = 10;
	char *tmp;
	int i;
	UART_PRINT("udp_read: %d, %d\n\r", socket_id, length);
	if(count-- >0)
	{
		UART_PRINT("udp_read: memcpy %x %x %d \n\r", buffer, tmp, length);
		buffer[0] = 0x27;
		buffer[1] = 0x20;
		buffer[2] = 0x27;
		buffer[3] = 0x20;
		buffer[1014] = 0x18;
		//memcpy(buffer, tmp, length);
		return length;	
		//UART_PRINT("SEND RESPONSE 4 %d\n\r", ring_buffer_send->end);
		//RingBuffer_Write(ring_buffer_send, TCP_READ_RESPONSE1, 1024);
		//UART_PRINT("SEND RESPONSE 5 %d\n\r", ring_buffer_send->end);
		//RingBuffer_Write(ring_buffer_send, TCP_READ_RESPONSE2, 1024);
	}else
	{
		count = 10;
		return -1;
	}
	return 0;
#else
	int recv_len = 0;
	int ret_val = 0;
	struct timeval tv;
	struct fd_set fdset;

	return 0;
#endif
}
#endif
/*
* purpose: write UDP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: write length
* return: -1: error
*		  >0: real length write
*/
int udp_write(int socket_id, char *buffer, int length)
{
#if (UINIT_TEST_IOS)
	UART_PRINT("udp_write: %d, %d\n\r", socket_id, length);
	return length;
#else
	int32_t ret_val = -1;
	int32_t remain_len = length;

	if (buffer == NULL) {
		UART_PRINT("udp_write: buffer is NULL\n\r");
		return -1;
	}

	while(remain_len > 0) {
		ret_val = send(socket_id, buffer + (length - remain_len), remain_len, 0);

		if (ret_val == 0) {
			UART_PRINT("udp_write: end\r\n");
			// Incase not send all data
			length -= remain_len;
			break;
		}
		else if (ret_val > 0) {
			UART_PRINT("udp_write: send %d bytes\r\n", ret_val);
			remain_len -= ret_val;
		}
		else {
			UART_PRINT("udp_write: send error %d\r\n", ret_val);
			// return error value
			remain_len = ret_val;
			break;
		}
	}

	UART_PRINT("udp_write: %d, %d\n\r", socket_id, length);
	return length;
#endif
}

#define PING_INTERVAL_TIME					(100)
#define PING_REQ_TIMEOUT					(500)
#define PING_ATTEMPT						(3)
#define INTERPRETER_PING_SIZE				(100)
#define PING_FLAG           				0
#define PING_SUCCESSFUL						"Ping successful"
#define PING_REPORT							"rtt min/avg/max/mdev = %u/%u/%u/%u ms"
/******PING*****/
/*
* purpose: Ping
* argument: host: server host name
*			response: receive buffer
* return: -1: error
*		  >0: real length response
*/
int ping(const char *host, char *response, int response_sz)
{
#if (UINIT_TEST_IOS)
	char a[32] = "ping testing done";
	UART_PRINT("ping : %s\n\r", host);
	memcpy(response, a, strlen(a));
	//sprintf(response, "ping testing done\n\r");
	return strlen(a);
#else
    long lRetVal = -1;
    
	UART_PRINT("ping : %s  len=%d\n\r", host, strlen(host));
{
    SlPingStartCommand_t PingParams;
    SlPingReport_t PingReport;
    unsigned long ulIpAddr;


    lRetVal = sl_NetAppDnsGetHostByName((signed char *) host
    									, strlen(host)
    									, &ulIpAddr
    									, SL_AF_INET);
    ASSERT_ON_ERROR(lRetVal); 
    UART_PRINT("Got ip\r\n");

	PingParams.Flags                  = 0;
    PingParams.Ip                     = ulIpAddr;
    PingParams.PingIntervalTime       = PING_INTERVAL_TIME;
    PingParams.PingRequestTimeout     = PING_REQ_TIMEOUT;
    PingParams.PingSize               = INTERPRETER_PING_SIZE;
    PingParams.TotalNumberOfAttempts  = PING_ATTEMPT;

    lRetVal = sl_NetAppPingStart(&PingParams,SL_AF_INET,&PingReport,NULL);
    ASSERT_ON_ERROR(lRetVal); 
    UART_PRINT("Ping done, PacketsSent=%d, PacketsReceived=%d\r\n", PingReport.PacketsSent, PingReport.PacketsReceived);

    if(PingReport.PacketsSent == PingReport.PacketsReceived)
    {
        UART_PRINT(PING_SUCCESSFUL);
        lRetVal = snprintf(response, response_sz, PING_REPORT, PingReport.MinRoundTime
        											, PingReport.AvgRoundTime
        											, PingReport.MaxRoundTime
        											, PingReport.TestTime);
    }
}
	UART_PRINT(response);
	return lRetVal;
#endif
}
