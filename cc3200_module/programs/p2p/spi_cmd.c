#include <string.h>
#include "spi_cmd.h"
#include "socket.h" // convert to network byte order


// #define DEBUG_SPI_CMD
#ifdef DEBUG_SPI_CMD
#if (PLATFORM == CC3200)
#include "uart_if.h"
#include "common.h"
// FOR CC3200
#define printf 		UART_PRINT
#else
// #define printf
#endif // PLATFORM
#else
#define printf
#endif // DEBUG_SPI_CMD

#define VALIDATE_PARAM      "VLVL"
#ifdef SPI_CMD_11_12
static void dump_data(uint8_t *data, size_t data_sz) {
	size_t i;
	printf("DUMP DATA---start\r\n");
	for (i = 0; i < data_sz; i++) {
		printf("%02x ", data[i]);
	}
	printf("DUMP DATA---end\r\n");
}
#define SPI_CMD_PRINT_FORMAT	"\
packet_type: %d\r\n\
end_flag: %d\r\n\
err_type: 0x%04x\r\n\
err_value: 0x%04x\r\n\
validate: 0x%08x\r\n\
op: %08x\r\n\
packet_seq: %u\r\n\
packet_index: %u\r\n\
packet_len: %u\r\n\
cmd_seq: %u\r\n\
cmd_id: %u\r\n"
static void dump_spi_cmd(spi_cmd_t *cmd) {
	if (cmd == NULL) {
		return;
	}

	printf(SPI_CMD_PRINT_FORMAT, cmd->header->packet_type
								   , cmd->header->end_flag
								   , cmd->header->err_type
								   , cmd->header->err_value
								   , cmd->header->validate
								   , cmd->header->op
								   , cmd->header->packet_seq
								   , cmd->header->packet_index
								   , cmd->header->packet_len
								   , cmd->header->cmd_seq
								   , cmd->header->cmd_id
								    );

}
uint8_t UT_send_data[1024];
uint8_t UT_recv_data[2048];
int32_t unittest_cmd(void) {
	int i = 0;
	spi_cmd_t parsed_cmd;
	int32_t ret_val = 0;

	for (i = 0; i < 10; i++) {
		memset(UT_send_data, 0xFA, sizeof(UT_send_data));
		memset(UT_recv_data, 0x00, sizeof(UT_recv_data));

		cmd_factory(UT_recv_data, sizeof(UT_recv_data)
			, UT_send_data, sizeof(UT_send_data)
			, SPI_CMD_GET_PS_KEY, SPI_CMD_REQUEST, 0, 0);
		dump_data(UT_recv_data, 64);
		ret_val = cmd_parse(&parsed_cmd, UT_recv_data);
		printf("cmd_parse ret_val=%d\r\n", ret_val);
		dump_spi_cmd(&parsed_cmd);
	}

	return 0;
}
// NOTE: change in_data
int32_t cmd_parse(spi_cmd_t *cmd, uint8_t *in_data) {
	int32_t ret_val = 0;

	if (in_data == NULL || cmd == NULL) {
		return -2;
	}
	// Convert
	cmd->header = (spi_cmd_header_t *)in_data;
	cmd->cmd_payload = in_data + sizeof(spi_cmd_header_t);
	// Convert to host byte order
	cmd->header->err_type  		= ntohs(cmd->header->err_type);
	cmd->header->err_value 		= ntohl(cmd->header->err_value);
	cmd->header->op 			= ntohl(cmd->header->op);
	cmd->header->packet_seq 	= ntohl(cmd->header->packet_seq);
	cmd->header->packet_index 	= ntohs(cmd->header->packet_index);
	cmd->header->packet_len 	= ntohs(cmd->header->packet_len);
	cmd->header->cmd_seq 		= ntohs(cmd->header->cmd_seq);
	cmd->header->cmd_id 		= ntohs(cmd->header->cmd_id);

	// Validate indata
	ret_val = memcmp((uint8_t *) & (cmd->header->validate), VALIDATE_PARAM
	                 , sizeof(cmd->header->validate));
	if (ret_val != 0) {
		return -3;
	}

	return ret_val;
}

/*******************************************************************************
* @brief: generate command, packet sequence counter, index package
* @param[out]: cmd  -  contain cmd 
* @param[in]: spi_cmd_sz  -  size of outbuffer
* @param[in]: payload - data in, parameter of command
* @param[in]: payload_sz - buffer size
* @param[in]: cmd_id - type of command
* @param[in]: cmd_type - request/response
* @param[in]: response_seq - generate response for command
* @param[in]: op - addtional op 
* @ret:       used bytes
*/
size_t cmd_factory(uint8_t *cmd, size_t cmd_sz, uint8_t *payload
                   , size_t payload_sz, uint32_t cmd_id, uint8_t cmd_type
                   , uint16_t response_seq, uint32_t op) {
	size_t used_bytes = 0;
	size_t max_data_per_packet = 0;
	uint16_t cur_cmd_seq = 0;
	int32_t number_of_packet = 0;
	int32_t extra_packet = 0;
	int32_t idx;
	spi_cmd_t new_cmd;

	if (cmd == NULL || payload == NULL) {
		return 0;
	}

	// FIXME: estimate size and number of packet
	// estimate max payload size of one packet
	max_data_per_packet = MAX_CMD_LEN - sizeof(spi_cmd_header_t);
	extra_packet = (payload_sz%max_data_per_packet)?1:0;
	number_of_packet = payload_sz / max_data_per_packet
					+ extra_packet;
	printf("max_data_per_packet=%d, payload_sz=%d, number_of_packet=%d, extra_sz=%d, origin=%d, final=%d\r\n"
		, max_data_per_packet
		, payload_sz
		, number_of_packet
		, (payload_sz%max_data_per_packet)?1:0
		, payload_sz / max_data_per_packet
		, (payload_sz / max_data_per_packet) + (payload_sz%max_data_per_packet)?1:0
		);

	// Check buffer len
	if (cmd_sz < (payload_sz + sizeof(spi_cmd_header_t)*number_of_packet)) {
		return 0;
	}

	memset(cmd, 0x00, cmd_sz);
	memset((uint8_t *)&new_cmd, 0x00, sizeof(spi_cmd_t));
	static uint16_t packet_seq = 0;
	uint16_t packet_index = 0;
	static uint32_t cmd_seq = 0;

	if (cmd_type == SPI_CMD_RESPONSE) {
		cur_cmd_seq = response_seq;
	}
	else {
		cur_cmd_seq = cmd_seq;
		cmd_seq++;
	}

	for (idx = 0; idx < number_of_packet; idx++) {
		int32_t copy_sz = 0;
		// estimate size
		copy_sz = (((idx + 1) * max_data_per_packet) > payload_sz)?
				   (payload_sz - idx*max_data_per_packet):max_data_per_packet;

		new_cmd.header = (spi_cmd_header_t *)((uint8_t *)cmd + idx*MAX_CMD_LEN);
		new_cmd.cmd_payload = (uint8_t *)(new_cmd.header) + sizeof(spi_cmd_header_t);

		// FILL IN DATA
		// Header
		new_cmd.header->packet_type 	= cmd_type;	
		new_cmd.header->end_flag 		= 1;	// currently not support
		new_cmd.header->err_type 		= htons(0x1234);	// currently not support
		new_cmd.header->err_value 		= htonl(0x2345);	// currently not support
		memcpy((char *)&(new_cmd.header->validate), VALIDATE_PARAM,sizeof(VALIDATE_PARAM));
		// new_cmd.header->validate = ;	
		new_cmd.header->op 				= htonl(op);	
		new_cmd.header->packet_seq 		= htonl(packet_seq);	
		new_cmd.header->packet_index 	= htons(packet_index); // convert to network order
		new_cmd.header->packet_len 		= htons(copy_sz);	
		new_cmd.header->cmd_seq 		= htons(cur_cmd_seq);	
		new_cmd.header->cmd_id 			= htons(cmd_id);	 // FOR TEST ONLY
		used_bytes += sizeof(spi_cmd_header_t);
		// Payload
		memcpy(new_cmd.cmd_payload, payload + idx*max_data_per_packet, copy_sz);
		used_bytes += copy_sz;

		// prepare for next packet	
		packet_seq++;
		packet_index++;
	}
	return used_bytes;
}
#endif
// int32_t spi_cmd_handler();