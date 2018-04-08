#ifndef __SPI_CMD_H__
#define __SPI_CMD_H__
#include <stdint.h>
#include <stdlib.h>

#if (PLATFORM == CC3200)
#include "spi_slave_handler.h"
#define MAX_CMD_LEN			DMA_SIZE	
#else
#define MAX_CMD_LEN			1024
#endif

enum command_id {
	SPI_CMD_GET_PS_KEY,
    SPI_CMD_STOP_STREAMING,
    SPI_CMD_UPLOAD_DONE,

	SPI_CMD_MAX_ID
};

// COMMAND
typedef struct spi_cmd_header
{
	uint8_t 	packet_type;
	uint8_t 	end_flag;	// Last packet
	uint16_t	err_type;	// Error checking type
	uint32_t	err_value;	// Error checking value
	uint32_t	validate; 	// VLVL
	uint32_t 	op;			// Extra flag for parsing
	uint32_t 	packet_seq;	// Increase for each packet
	uint16_t 	packet_index; // Index in packet group
	uint16_t 	packet_len;	// Payload len
	uint16_t 	cmd_seq;	// Increase for each cmd_id
	uint16_t 	cmd_id;		// cmd id
} spi_cmd_header_t __attribute__((aligned(16)));

typedef struct spi_cmd
{
	spi_cmd_header_t *header;
	uint8_t *cmd_payload;
} spi_cmd_t __attribute__((aligned(16)));

int32_t cmd_parse(spi_cmd_t *cmd, uint8_t *in_data);
size_t cmd_factory(uint8_t *cmd, size_t cmd_sz, uint8_t *payload, size_t payload_sz, uint32_t cmd_id, uint8_t cmd_type, uint16_t response_seq, uint32_t op);
int32_t unittest_cmd(void);
// No need to use SPI_PACKET_TYPE 11/12
//#define SPI_CMD_11_12
#endif