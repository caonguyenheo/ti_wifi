#ifndef __SPI_SLAVE_HANDLER_H__
#define __SPI_SLAVE_HANDLER_H__
#include <stdint.h>

#define AK_SPI_TEST             0

enum SPI_PACKET_TYPE {
	CMD_VERSION = 1,
	CMD_UDID = 2,
	DATA_STREAMING = 3,
	CMD_STATUS = 4,
	CMD_GET_PACKET = 5,
	CMD_RESET = 6,
	CMD_TCP = 7,
	CMD_UDP = 8,
	CMD_PING = 9,
	CMD_GET_RESP = 10,
	SPI_CMD_REQUEST = 11,
	SPI_CMD_RESPONSE = 12,
	DATA_ACK = 13,
	CMD_AKOTA = 0x15,
	CMD_UPLOAD = 0x16,
	FILE_ACK = 0x1D,
	CMD_FILEUPLOAD = 0x1E,
	CMD_FLICKER = 0x8E,
	CMD_IMAGE_FLIP = 0x8F,
	CMD_BITRATE = 0x90,
	CMD_FRAMERATE = 0x91,
	CMD_RESOLUTION = 0x92,
	CMD_AESKEY = 0x93,
	CMD_CDS = 0x94,
	CMD_NTP = 0x97,
	CMD_VER = 0x98,
	CMD_DINGDONG = 0x99,
	CMD_V2UDID = 0x9A,
	CMD_TOKEN = 0x9B,
	CMD_UPLOADURL = 0x9C,
	CMD_VOICEPROMPT = 0x9F,
	CMD_VOLUME = 0xA1,
	CMD_FTEST = 0xA4,
	MAX_PACKET_TYPE
};
enum SPI_SET_GET_TYPE {
	GET_ID =  0x01,
	SET_ID = 0x02,
};

enum SPI_GET_RESPONSE_TYPE {
	RESPONSE_FLICKER = 0x0E,
	RESPONSE_IMAGE_FLIP = 0x0F,
	RESPONSE_IMAGE_BITRATE  = 0x10,
	RESPONSE_FRAMERATE = 0x11,
	RESPONSE_RESOLUTION = 0x12,
	RESPONSE_AESKEY = 0x13,
	RESPONSE_VERSION = 0x18,
	RESPONSE_V2UDID = 0x1A,
	RESPONSE_TOKEN = 0x1B,
	RESPONSE_UPLOADURL = 0x1C,
	RESPONES_AK_KL = 0x20,
	RESPONSE_FTEST = 0x24,
};
#define DMA_SIZE                1024

//int read_ringbuf(uint8_t* data);
void spiSlaveHandleTask(void * param);
int set_response_flag(int resp);
void spi_init();
void spi_deinit(void);
#if (AK_SPI_TEST)
int spi_send_data(uint8_t *data, int len);
#endif
extern int spi_thread_pause;
extern int motionupload_data_pos; // Current position of send data
void reset_buffer();
#endif
