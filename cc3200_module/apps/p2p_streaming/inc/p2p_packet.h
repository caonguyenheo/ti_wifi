#ifndef __P2P_PACKET_H__
#define __P2P_PACKET_H__
#include <stdint.h>
#define MAX_MEDIA_DATA_LEN			999
#define MAX_REQUEST_STRING_LEN		1009
#define MEDIA_TYPE_BLOCK			5
#define PID_BLOCK_OF_COMMAND_FRAME	6
#define PID_BLOCK_OF_ACK_FRAME	 	6
#define PID_BLOCK_OF_MEDIA_FRAME	16
#define PID_OF_FRAME_LENGTH			4

int32_t frame_2_pid(char *input);
int32_t data_2_mediatype(char *data);
uint32_t ack_2_pid_ack(char *data);
uint32_t command_2_pid(char *data);
// unit test
int32_t test_packet(void);
#endif // __P2P_PACKET_H__