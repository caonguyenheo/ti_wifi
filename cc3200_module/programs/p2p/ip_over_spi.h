#ifndef IP_OVER_SPI_
#define IP_OVER_SPI_
/*Data Structure: 1024 spi byte transfer
[Transport Type: 1byte] [Transport ID: 1byte] [Transport Control: 1byte] [Data Length: 2 bytes] [Fragment Flag: 1byte] [Reserved: 1byte] [Transport Data: max 1017 bytes]
[Transport Type: 1byte] : 0x07:TCP, 0x08:UDP, 0x09:Ping
[[Transport ID: 1byte]]: Socket or Session ID
[Transport Control: 1byte]: 0x00: open, 0x01: close, 0x02: send, 0x03: receive
[Transport Error Code: 1byte]: 0x00: no error, -1: some thing fail
[Data Length: 2 bytes]: Actual Length of [Transport Data]
[Fragment Flag: 1byte]: 0: not the last fragment packet, 1: the last fragment packet
[Reserved: 1byte]: In some special case of future
[Transport Data: max 1017 bytes]: The real transport data*/
#define TRANSPORT_TRANSFER_LENGTH	1024
#define TRANSPORT_DATA_LENGTH		1015
typedef struct IPOVERSPI_TRANSFER_t
{
	char transport_type;
	char transport_id;
	char transport_control;
	char transport_err_code;
	char transport_direction;
	char data_length[2];
	char fragment_flag;
	char reserved;
	char transport_data[TRANSPORT_DATA_LENGTH];
} IPOVERSPI_TRANSFER;
#define MAX_SESSION		(10)
typedef struct IPOVERSPI_SESSION_t
{
	/*Data structer*/
	IPOVERSPI_TRANSFER t_data;
	/*Control flag*/
	int is_used;
	int process_done;
	/*session data*/
	char *session_write_data;
	int session_write_length;
	char *session_read_data;
	int session_read_length;// the data length need to be read

	int session_transfer_length; // the actual data length is transfered
	
} IPOVERSPI_SESSION;
/*Transfer type = command id*/
#define IOS_TYPE_TCP 		0x07
#define IOS_TYPE_UDP 		0x08
#define IOS_TYPE_PING 		0x09
#define IOS_TYPE_GET_RESP	0x0A

/*Transport control*/
#define IOS_CONTROL_OPEN	0x00
#define IOS_CONTROL_CLOSE	0x01
#define IOS_CONTROL_SEND	0x02
#define IOS_CONTROL_RECEIVE	0x03

/*Transport error code*/
#define IOS_ERROR_OK		(0)
#define IOS_ERROR_FAIL		(-1)

/*Transport Direction*/
#define IOS_DIRECTION_H2W	(0)
#define IOS_DIRECTION_W2H	(1)

/*Fragment Flag*/
#define IOT_FRAGMENT_LAST	(1)
#define IOT_FRAGMENT_CONT	(0)
int ios_packet_create(IPOVERSPI_TRANSFER * data, char type, char id, char control, char err_code, char direction, int len, char fragment, char *buffer);
int ios_packet_parse(IPOVERSPI_TRANSFER *data, char *type, char *id, char *control, char *err_code, char *direction, int *len, char *fragment, char **buffer);
int int_2_array2byte(int num, unsigned char *array);
int array2byte_2_int(unsigned char *array);

/*****IP over SPI Library Init*****/
int ios_init();
int ios_deinit();

 /******TCP*****/
/*
* purpose: Open TCP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  >0: socket id
*/
int tcp_open(const char *host, int port);

/*
* purpose: Close TCP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  0: successful
*/
// int tcp_close(int socket_id);

/*
* purpose: Read TCP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: read length
* return: -1: error
*		  >0: real length read
*/
int tcp_read(int socket_id, char *buffer, int length);

/*
* purpose: write TCP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: write length
* return: -1: error
*		  >0: real length write
*/
int tcp_write(int socket_id, char *buffer, int length);
 
 /******UDP*****/
 /*
* purpose: Open UDP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  >0: socket id
*/
int udp_open(const char *host, int port);

/*
* purpose: Close UDP session
* argument: host: server host name
*			port: server port
* return: -1: error
*		  0: successful
*/
// int udp_close(int socket_id);

/*
* purpose: Read UDP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: read length
* return: -1: error
*		  >0: real length read
*/
// int udp_read(int socket_id, char *buffer, int length);

/*
* purpose: write UDP
* argument: socket_id: socket id
*			buffer: read data buffer
*			length: write length
* return: -1: error
*		  >0: real length write
*/
int udp_write(int socket_id, char *buffer, int length);

/******PING*****/
/*
* purpose: Ping
* argument: host: server host name
*			response: receive buffer
*			response_sz: receive buffer size
* return: -1: error
*		  >0: real length response
*/
int ping(const char *host, char *response, int response_sz);
#endif
