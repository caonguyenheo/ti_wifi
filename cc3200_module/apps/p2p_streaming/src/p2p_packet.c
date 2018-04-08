//****************************************************************************
//
//! \addtogroup p2p
//! @{
//
//****************************************************************************
#include "p2p_packet.h"
#include <string.h>
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "socket.h"

//*****************************************************************************
//                      PUBLIC FUNCTION
//*****************************************************************************
int32_t test_packet(void) {
	char test_input_1[128] = {'V','L','V','L',0x00,10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00};
	char test_input_2[128] = {'V','L','V','L',0x00,50,0x00,0x01,0x02,0x03,0x00,0x00};
	UART_PRINT("RESULT=%d\n"			,frame_2_pid(test_input_1));
	UART_PRINT("RESULT_MEDIA_TYPE=%d\n"	,data_2_mediatype(test_input_1));
	UART_PRINT("RESULT_ACK=%d\n"		,ack_2_pid_ack(test_input_2));
	UART_PRINT("RESULT_COMMAND=%d\n"	,command_2_pid(test_input_2));
	return 0;
}

static inline int32_t get_pid(char *data, uint16_t block_num) {
	int32_t pid = 0;
	memcpy(&pid, (char *)data + block_num, sizeof(pid));
	return ntohl(pid);
}

// frame_2_pid(char *input) (Khanh)
// 	Get current pid from current packet
// 	input: 'V','L','V','L',0x00,10,,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00
// 	output: 16
int32_t frame_2_pid(char *data) {
	return get_pid(data, PID_BLOCK_OF_MEDIA_FRAME);
}

// data_2_mediatype(data) (Khanh)
// 	Get media type from current packet
// 	input: 'V','L','V','L',0x00,10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00
// 	output: 10
int32_t data_2_mediatype(char *data) {
	uint8_t media_type = 0;
	memcpy(&media_type, data + MEDIA_TYPE_BLOCK, sizeof(media_type));
	return media_type;
}

// uint32 ack_2_pid_ack(data)
// 	Return PID of ACK packet
// 	input: 'V','L','V','L',0x00,50,0x00,0x01,0x02,0x03,336(0x00)
// 	output: 66051
uint32_t ack_2_pid_ack(char *data) {
	return get_pid(data, PID_BLOCK_OF_ACK_FRAME);
}

// uint32 command_2_pid(data)
// 	Return PID of HTTP command packet
// 	input: 'V','L','V','L',0x00,10,0x00,0x01,0x02,0x03,16(0x00)
// 	output: 66051
uint32_t command_2_pid(char *data) {
	return get_pid(data, PID_BLOCK_OF_COMMAND_FRAME);
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
