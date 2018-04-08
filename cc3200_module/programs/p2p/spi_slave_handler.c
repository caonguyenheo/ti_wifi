#include "spi_slave_handler.h"
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

// SimpleLink include
#include "simplelink.h"

// Free-RTOS/TI-RTOS include
#include "osi.h"
#include "common.h"

// for DMA
#include "udma.h"
#include "udma_if.h"
#include "hw_mcspi.h"

#include <stdlib.h>
#include "ringbuffer.h"
#include "system.h"

// TEST
#include "date_time.h"
#include "FreeRTOS.h"
// #include "task.h"
#include "semphr.h"

// for standard lib
#include <stdint.h>
#include "ip_over_spi.h"
#include "spi_cmd.h"
//
#include "date_time.h"
#include "cc_types.h"
#include "rtc_hal.h"
#include "cc3200_system.h"

//*****************************************************************************
//                 MACRO
//*****************************************************************************

#define UNITTEST_SPI            0

/* SPI_IF_BIT_RATE must be an integer divisor of 20000000 */
#define SPI_20_MHZ              200000000
#define SPI_10_MHZ              200000000 / 2
#define SPI_2_MHZ               200000000 / 10
// #define SPI_IF_BIT_RATE         SPI_20_MHZ
#define SPI_IF_BIT_RATE         SPI_20_MHZ
#define TR_BUFF_SIZE            1024
#define MAX_MSG_LENGTH          256

#define BUFFER_SIZE             16
#define RING_BUF_LENGTH (1024*BUFFER_SIZE)

extern char p2p_key_char[];
int packet_process(char *input, int length);
int ios_handler(char packet_type, char *in_data, int in_len);
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static unsigned char tx_buffer[DMA_SIZE]={0};
static unsigned char rx_buffer[DMA_SIZE]={0};
static char data_buf[DMA_SIZE];
//RingBuffer rb_recv;
//RingBuffer rb_send;
//RingBuffer* ring_buffer_receive;
//RingBuffer* ring_buffer_send;
//static SemaphoreHandle_t xBinarySemaphore;
int response_flag = 0;
char* boot_mem=(char *)0x20000000;
int g_spi_recv_pos_write=0;
int g_spi_recv_pos_read=0;
#define SPI_RECV_BUF_LEN (BUFFER_SIZE*1024)
int g_spi_send_pos_write=0;
int g_spi_send_pos_read=0;
#define SPI_SEND_BUF_LEN ((BUFFER_SIZE/2)*1024)
char* spi_send_ptr=NULL;
int spi_thread_pause=1;
// static SemaphoreHandle_t ring_buffer_sem;
//*****************************************************************************
//
//! Print buffer
//!
//! This function print data item of the buffer
//!
//! \return None.
//
//*****************************************************************************
int set_response_flag(int resp)
{
    response_flag = resp;
}
int get_response_flag()
{
    return response_flag;
}
static void print_buffer(uint8_t * buffer, _u32 bufLen)
{
    int i;
    //UART_PRINT("\r\n");
    for (i = 0; i < bufLen; i++)
    {
        UART_PRINT("0x%02X,", buffer[i]);
    }
    // UART_PRINT("%02X", buffer[0]);
    UART_PRINT("\n\r");
}

static void spi_transfer(uint8_t* tx, uint8_t* rx);
void spi_init(void);
//*****************************************************************************
//
//! SPI Slave Interrupt handler
//!
//! This function is invoked when SPI slave has its receive register full or
//! transmit register empty.
//!
//! \return None.
//
//*****************************************************************************
//static int is_reading = 0;
//static int is_writing = 0;
static void slave_int_handler()
{
    uint32_t status = MAP_SPIIntStatus(GSPI_BASE, true);
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    /* 'Give' the semaphore to unblock the task. */
    // xSemaphoreGiveFromISR( xBinarySemaphore, &xHigherPriorityTaskWoken );

    MAP_SPIIntClear(GSPI_BASE, SPI_INT_EOW);
#if (0)
    //update the ring_buffer s'end
    // WRITING
    // UART_PRINT("IS_WRITING=%d\r\n", is_writing);
    if (is_writing) {
        RingBuffer_AdvanceWriteIndex(ring_buffer_receive, DMA_SIZE);
        is_writing = 0;
    }

    // FIXME: should check availabe space
    {
        int32_t size1, size2;
        int32_t write_byte;
        char *data2 = NULL;
        write_byte = RingBuffer_GetWriteRegions(ring_buffer_receive, DMA_SIZE ,
                                                (void **)&recv_buff, &size1, (void **) &data2, &size2);
        if (write_byte == DMA_SIZE) {
            is_writing = 1;
        }
        else {
            // BUFFER is full
            UART_PRINT(".");
            recv_buff = rx_buffer;
        }
    }
#else
    int32_t write_byte = 0;
    // UART_PRINT("_%d_", RingBuffer_GetReadAvailable(ring_buffer_receive));
    //UART_PRINT("-%d_%d_%d_%d-\r\n", rx_buffer[0],rx_buffer[1],rx_buffer[1022],rx_buffer[1023]);
    int i;
    unsigned short usCsErr=0;
    char *pPacket=rx_buffer;
    for(i = 0; i < 1022; i++){
        usCsErr += *(pPacket + i);
    }
    char cHighByte = 0;
    char cLowByte = 0;

    cHighByte = (usCsErr >> 8) & 0xFF;
    cLowByte = usCsErr & 0xFF;
    //UART_PRINT("\r\nusCs: %x, b1022: %x, b1023:%x, packet_type:0x%x\r\n", usCsErr, *(pPacket + 1022), *(pPacket + 1023), packet_type);
    if((cHighByte != *(pPacket + 1022)) || (cLowByte != *(pPacket + 1023))){
    	UART_PRINT("SPI_Err\r\n");
	}else if (rx_buffer[0]!=0){
		if (boot_mem[g_spi_recv_pos_write]==0){
			memcpy(boot_mem+g_spi_recv_pos_write,rx_buffer,1024);
			g_spi_recv_pos_write=(g_spi_recv_pos_write+1024)%SPI_RECV_BUF_LEN;
		}
	}


    	//write_byte = RingBuffer_Write(ring_buffer_receive, rx_buffer, DMA_SIZE);
    /*if (write_byte != DMA_SIZE) {
        UART_PRINT(".%d", write_byte);
    }*/
    // if (write_byte == 0) {
    //     // UART_PRINT("Buffer is full ---> flush\r\n");
    //     UART_PRINT(".");
    // }
    //memset(rx_buffer, 0x00, sizeof(rx_buffer));
    rx_buffer[0]=0;
#endif

    int ret_val;
    memset(tx_buffer, 0x00, sizeof(tx_buffer));
    //ret_val = RingBuffer_Read(ring_buffer_send, tx_buffer, DMA_SIZE);
    if ((spi_send_ptr+g_spi_send_pos_read)[0]!=0){
        memcpy(tx_buffer,spi_send_ptr+g_spi_send_pos_read,DMA_SIZE);
        memset(spi_send_ptr+g_spi_send_pos_read, 0x00, DMA_SIZE);
        g_spi_send_pos_read=(g_spi_send_pos_read+DMA_SIZE)%SPI_SEND_BUF_LEN;
        //UART_PRINT("g_spi_send_pos_read%d\n\r",g_spi_send_pos_read);
    }
    // Config DMA
    spi_transfer(tx_buffer, rx_buffer);

    if (!(status & SPI_INT_EOW))
    {
        UART_PRINT("Error: Unexpected SPI interrupt!\n\r");
    }
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.
    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports. The portEND_SWITCHING_ISR() macro is provided as part of
    the Corte M3 port layer for this purpose. taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*void reset_buffer() {
    memset(boot_mem, 0x00, RING_BUF_LENGTH);
    if (spi_send_ptr!=NULL)
        memset(spi_send_ptr, 0x00, SPI_SEND_BUF_LEN);
}*/
int spi_init_buf_status=0;
void init_buffer() {
    if (spi_init_buf_status)
        return;
    //uint32_t ret_val = -1;
    //uint8_t *ptr_buffer = NULL;

    // Recv buf
    // USING BOOT section for spi buffer
    memset(boot_mem, 0x00, RING_BUF_LENGTH);
    // Send buf
    spi_send_ptr = malloc(SPI_SEND_BUF_LEN);
    memset(spi_send_ptr, 0x00, SPI_SEND_BUF_LEN);
    //ret_val = RingBuffer_Init( &rb_send, RING_BUF_LENGTH/2, ptr_buffer);
    //UART_PRINT("\r\nret_val_rb_send = %d\r\n", ret_val);
    //ring_buffer_send = &rb_send;
    spi_init_buf_status = 1;
}

void spi_init()
{
    //
    // Reset the peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    //
    // Set Tx buffer index
    //

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    UDMAInit();

    //
    // Configure SPI interface
    //
//    UART_PRINT("SPI_IF_BIT_RATE %d\r\n", SPI_IF_BIT_RATE);
    MAP_SPIConfigSetExpClk(GSPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           SPI_IF_BIT_RATE, SPI_MODE_SLAVE, SPI_SUB_MODE_0,
                           (SPI_HW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_ON |
                            SPI_CS_ACTIVELOW |
                            SPI_WL_32));

    MAP_uDMAChannelAssign( UDMA_CH6_GSPI_RX );
    MAP_uDMAChannelAssign( UDMA_CH7_GSPI_TX );

    MAP_uDMAChannelAttributeDisable( UDMA_CH6_GSPI_RX, UDMA_ATTR_ALTSELECT );
    MAP_uDMAChannelAttributeDisable( UDMA_CH7_GSPI_TX, UDMA_ATTR_ALTSELECT );
    MAP_uDMAChannelAttributeEnable( UDMA_CH6_GSPI_RX, UDMA_ATTR_HIGH_PRIORITY );
    MAP_uDMAChannelAttributeEnable( UDMA_CH7_GSPI_TX, UDMA_ATTR_HIGH_PRIORITY );
    //
    // Register Interrupt Handler
    //
    MAP_SPIIntRegister(GSPI_BASE, slave_int_handler);
    // osi_InterruptRegister(INT_GSPI, (P_OSI_INTR_ENTRY)slave_int_handler, INT_PRIORITY_LVL_1);
    MAP_SPIWordCountSet(GSPI_BASE, DMA_SIZE / 4);
    MAP_SPIFIFOLevelSet(GSPI_BASE, 1, 1);
    MAP_SPIFIFOEnable(GSPI_BASE, SPI_RX_FIFO);
    MAP_SPIFIFOEnable(GSPI_BASE, SPI_TX_FIFO);
    MAP_SPIDmaEnable(GSPI_BASE, SPI_RX_DMA);
    MAP_SPIDmaEnable(GSPI_BASE, SPI_TX_DMA);
    MAP_SPIIntEnable(GSPI_BASE, SPI_INT_EOW);
    //
    // Enable Interrupts
    //
//  MAP_SPIIntEnable(GSPI_BASE,SPI_INT_RX_FULL|SPI_INT_TX_EMPTY);

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);
}

void spi_deinit(void) {
    MAP_SPIDisable(GSPI_BASE);
    memset(boot_mem, 0x00, RING_BUF_LENGTH);
    if (spi_send_ptr!=NULL)
        memset(spi_send_ptr, 0x00, SPI_SEND_BUF_LEN);
}

static void spi_transfer(uint8_t* tx, uint8_t* rx)
{
    MAP_SPIDisable(GSPI_BASE);
    UDMASetupTransfer(UDMA_CH6_GSPI_RX, UDMA_MODE_BASIC, DMA_SIZE / 4,
                      UDMA_SIZE_32, UDMA_ARB_1,
                      (void *)(GSPI_BASE + MCSPI_O_RX0), UDMA_SRC_INC_NONE,
                      rx, UDMA_DST_INC_32);

    UDMASetupTransfer(UDMA_CH7_GSPI_TX, UDMA_MODE_BASIC, DMA_SIZE / 4,
                      UDMA_SIZE_32, UDMA_ARB_1,
                      tx, UDMA_SRC_INC_32,
                      (void *)(GSPI_BASE + MCSPI_O_TX0), UDMA_DST_INC_NONE);
    MAP_SPIEnable(GSPI_BASE);
}





/// HEADER ///
#define SPI_PACKET_HEADER_LEN       1
#define SPI_PACKET_LEN_FIELD_LEN    2
#define SPI_MAX_PACKET_LEN          1019

int spi_cmd_handler(void *cmd_data, uint32_t cmd_len, uint16_t cmd_type
                    , void *cmd_response , uint32_t cmd_response_sz);
int spi_data_processing(void *spi_data, uint32_t spi_data_len, void *spi_response
                        , uint32_t spi_response_sz);
/// END HEADER ///

#define MAX_PARAM_LEN               128

//*****************************************************************************
//
//! spi_cmd_handler
//!
//! process spi command and return response if need
//!
//! \return  0 success, > 0  success and have response, < 0 failed
//
//*****************************************************************************
#ifdef SPI_CMD_11_12
int spi_cmd_handler(void *cmd_data, uint32_t cmd_len, uint16_t cmd_type
                    , void *cmd_response, uint32_t cmd_response_sz) {
    int32_t ret_val = 0;
    uint8_t cmd_param[MAX_PARAM_LEN] = {0};
    uint8_t response_buf[DMA_SIZE] = {0};
    spi_cmd_t parsed_cmd;
    if (cmd_data == NULL) {
        return -1;
    }

    ret_val = cmd_parse(&parsed_cmd, cmd_data);
    if (ret_val < 0) {
        ERR_PRINT(ret_val);
        return ret_val;
    }

    switch (parsed_cmd.header->cmd_id) {
    case SPI_CMD_GET_PS_KEY:
        UART_PRINT("SPI_CMD_GET_PS_KEY\r\n");
        memset(response_buf, 0x00, sizeof(response_buf));
        ret_val = cmd_factory(response_buf, sizeof(response_buf)
            , p2p_key_char, strlen(p2p_key_char)
            , parsed_cmd.header->cmd_id, SPI_CMD_RESPONSE, parsed_cmd.header->cmd_seq, 0);
        if (ret_val != 0) {
            UART_PRINT("SEND RESPONSE, ret_val = %d\r\n", ret_val);
            //ret_val = RingBuffer_Write(ring_buffer_send, (char *)response_buf, DMA_SIZE);
            ret_val = sendbuf_put_spi_buff(response_buf, DMA_SIZE);
            print_buffer(response_buf, 10);
            // UART_PRINT("out_buf=%d\r\n", ret_val);
        }
        break;
    default:
        break;
    }
    return ret_val;
}
#endif
//*****************************************************************************
//
//! spi_data_processing
//!
//! check and classify data type, if streaming data, send to p2p_main handle
//!
//! \return  0 success, > 0  success and have response, < 0 failed
//
//*****************************************************************************
int audio_count=0;
// Motion Uploading
#include "server_http_client.h"
int motionupload_available = 0; // TCP socket over SPI in progress
HTTPCli_Struct motionupload_pHttpClient;
char motionupload_host[40]={0}; // Host link (don't need url)
int motionupload_port=443; // Host port
char motionupload_secure=1; // 1 if secure SSL
int motionupload_bodylen=0; // Total body length of response
int motionupload_bodylen_current=0; // Current length of response
int motionupload_data_pos = 0; // Current position of send data
void motion_upload_close(void){
	if (motionupload_available)
		myCinHttpClose(&motionupload_pHttpClient);
	motionupload_available=0;
}
extern int spi_get_motion_file(char* spi_response);
extern int spi_set_motion_file();
// End motioni uploading
int test_count=0;
int mu_log_count=0;
extern int AK_KeepAlive_status;
int spi_data_processing(void *spi_data, uint32_t spi_data_len, void *spi_response
                        , uint32_t spi_response_sz) {
    uint8_t packet_type = MAX_PACKET_TYPE;
    int32_t ret_val = 0;

    // Trung add checksum data
    unsigned short usCsErr = 0;
    char cHighByte = 0;
    char cLowByte = 0;
    int i = 0;
    char *pPacket = (char *)spi_data;
    char acDebugCs[1024];
    int checksum_ok; // If checksum of SPI packet is OK, not all packets need to check seriously
    
    // UART_PRINT("b");
    // count packet
    /*
    static int packet_counter = 0;
    static unsigned int time_ms = 0;
    unsigned int current_ms = 0;
    if (packet_counter == 0) {
        time_ms = stopwatch_get_ms();
    }
    packet_counter++;


    current_ms = stopwatch_get_ms();
    if ((current_ms - time_ms) > 1000) {// 1s
        UART_PRINT("packet/s = %d\r\n", packet_counter);
        packet_counter = 0;
    }*/
    #if (UNITTEST_SPI)
    static int passed_cnt = 0;
    static int failed_cnt = 0;
    char expected_data[DMA_SIZE] = {0};
    {
        int i;
        for (i = 0; i < DMA_SIZE; i++) {
            expected_data[i] = i;
        }
    }
    if (memcmp(expected_data, spi_data, spi_data_len) != 0) {
        print_buffer((char *)spi_data, 16);
        failed_cnt++;
    }
    else {
        passed_cnt++;
    }
    UART_PRINT("SPI_TEST_RESULT: passed=%d, failed=%d\r\n", passed_cnt, failed_cnt);
    #endif

    char *ptr_data = (char *)spi_data;
    if (ptr_data == NULL) {
        ret_val = -1;
        UART_PRINT("\r\nError 0\r\n");
        goto exit_error;
    }

    // SPI packet verify
    // check packet type
    packet_type = *(ptr_data++);
    //UART_PRINT("\r\nPT=%02X ", packet_type);
    if (packet_type >= MAX_PACKET_TYPE) {
        ret_val = -1;
        //UART_PRINT("\r\nError 1, packet_type=%d, MAX_PACKET_TYPE=%d\r\n", packet_type, MAX_PACKET_TYPE);
        goto exit_error;
    }
    // TODO: add more machenism to check data interity
    // Trung add checksum
    for(i = 0; i < 1022; i++){
        usCsErr += *(pPacket + i);
    }
    cHighByte = (usCsErr >> 8) & 0xFF;
    cLowByte = usCsErr & 0xFF;
    //UART_PRINT("\r\nusCs: %x, b1022: %x, b1023:%x, packet_type:0x%x\r\n", usCsErr, *(pPacket + 1022), *(pPacket + 1023), packet_type);
    if((cHighByte != *(pPacket + 1022)) || (cLowByte != *(pPacket + 1023))){
        UART_PRINT("ERROR: Checksum failed! nusCs: %x, b1022: %x, b1023:%x\r\n", usCsErr, *(pPacket + 1022), *(pPacket + 1023));
        memset(acDebugCs, 0x00, 1024);
        pPacket = (char *)(spi_data + 1023 - 20);
        for(i=0; i < 20; i ++)
        {
            sprintf(acDebugCs + strlen(acDebugCs), "%02X ", *(pPacket + i));
        }
        UART_PRINT("The last 20 bytes: %s\r\n", acDebugCs);
        checksum_ok = 0;
    } else
        checksum_ok = 1;

    if (packet_type == DATA_STREAMING)  {
        int32_t p2p_packet_len = 0;
        p2p_packet_len = 0;
        p2p_packet_len |= (*(ptr_data++) & 0xFF) << 8;
        p2p_packet_len |= (*(ptr_data++) & 0xFF);
        if (p2p_packet_len > SPI_MAX_PACKET_LEN) {
            UART_PRINT("\r\nError 2\r\n");
            goto exit_error;
        }
        // UART_PRINT("Length = %d", p2p_packet_len);
        // print_buffer(ptr_data, 20);
        if ((ptr_data[5]/10)!=8){ //Not fast audio
            if ((test_count%10)==0){
	            if (ptr_data[5]==41){// Filedata
	            	i=(ptr_data[18]<<8)+ptr_data[19];
	            	UART_PRINT("S-PID%d ", i);
            	}
            }
            test_count++;
            packet_process(ptr_data, p2p_packet_len);
        }
        else{ //Fast audio
	        //if ((audio_count%AUDIO_DROP_PERIOD)!=0)
	            packet_process(ptr_data, p2p_packet_len);
	    	audio_count++;
        }
    } else if (packet_type >= IOS_TYPE_TCP && (packet_type <= IOS_TYPE_GET_RESP))
    {
        // ret_val = ios_handler(packet_type, (char *)spi_data, spi_data_len);
        UART_PRINT("\r\nWrong ios\r\n");
    } else if ((packet_type==CMD_UPLOAD) && checksum_ok){ // For uploading, need to make sure checksum is correct
		//UART_PRINT("MU: %02X %02X %02X %02X %02X %02X %02X\r\n", pPacket[1],pPacket[2],pPacket[3], pPacket[4],pPacket[5],pPacket[6],pPacket[7]); 
		if (pPacket[1]==0x01){ //Open socket
			int l_data_len=(pPacket[2]<<8)|pPacket[3];
			int l_secure=pPacket[4];
			int l_port=(pPacket[5]<<8)|pPacket[6];
			if ((l_data_len<=43) && (l_data_len>6)){
				pPacket[4+l_data_len]=0x00;
				UART_PRINT("MU: Check old Socket\r\n");
				if ((motionupload_available==0) || ((motionupload_available) && ((motionupload_secure != l_secure) || (motionupload_port != l_port) || (strlen(motionupload_host)<3) || (strstr(pPacket[4+3],motionupload_host)==NULL))))
				{
					UART_PRINT("MU: Socket now NA\r\n");
					if (motionupload_available){ //There is a socket, close it
						UART_PRINT("MU: Close socket\r\n");
						myCinHttpClose(&motionupload_pHttpClient);
					}
					motionupload_secure = l_secure;
					motionupload_port = l_port;
					memcpy(motionupload_host,pPacket+4+3,l_data_len-3);
					motionupload_host[l_data_len-3]=0x00;
					// Open new socket
					if (myCinHttpConnect(&motionupload_pHttpClient, motionupload_host, motionupload_port, motionupload_secure)<0){
						UART_PRINT("MU: Open socket fail\r\n");
						myCinHttpClose(&motionupload_pHttpClient);
						motionupload_available = 0;
					} else {
						UART_PRINT("MU: Open socket OK %s %d\r\n",motionupload_host,motionupload_port);
						motionupload_available = 1;
					}
				}
				if (motionupload_available){
					pPacket[1] = 0x02;
					set_down_anyka_cmd(pPacket,0);
					set_down_anyka_cmd(pPacket,0);
					motionupload_data_pos = 0;
				}
			}
		} else if (pPacket[1]==0x03){ //Send request
			int l_data_len=(pPacket[2]<<8)+pPacket[3];
			int l_position=(pPacket[4]<<24)|(pPacket[5]<<16)|(pPacket[6]<<8)|pPacket[7];
			int l_return;
			char l_byte;
			//UART_PRINT("Upload data %d %d\r\n",l_data_len,motionupload_data_pos);
			if ((l_data_len-4)>0){ // Data sending in progress
				if (l_position==motionupload_data_pos){
					l_return = HTTPCli_sendRequestBody(&motionupload_pHttpClient, pPacket+8, l_data_len-4);
					/*
					for (i=0;i<l_data_len-4;i++){
						l_byte=(pPacket+8)[i];
						if ((l_byte>=33)&&(l_byte<=126))
							UART_PRINT("%c", l_byte);
						}*/
					//UART_PRINT("MU: send %d bytes, response %d\r\n",l_data_len-4,l_return);
					UART_PRINT("MU: send pos %d bytes, response %d\r\n",motionupload_data_pos,l_return);
					if (l_position==0){
						*(pPacket+8+450)=0x00;
						UART_PRINT("MU: S header %s\r\n",pPacket+8);
					}
					if ((l_return>0) || (l_return<0)){
						if (l_return<0)
							UART_PRINT("MU: S body fails at %d\r\n",motionupload_data_pos);
						l_position+=(l_data_len-4);
						motionupload_data_pos=l_position;
					} else
						UART_PRINT("MU: S body zero\r\n");
				}
				//UART_PRINT("MU: wait pos %d\r\n", motionupload_data_pos);
				pPacket[1] = 0x04;
				pPacket[2] = (motionupload_data_pos>>24)&0xff;
				pPacket[3] = (motionupload_data_pos>>16)&0xff;
				pPacket[4] = (motionupload_data_pos>>8)&0xff;
				pPacket[5] = (motionupload_data_pos)&0xff;
				set_down_anyka_cmd(pPacket,0);
			} else if ((l_data_len-4)==0){ // No more data to send
				motionupload_bodylen_current = 0;
				motionupload_bodylen = 0;
				motionupload_data_pos = 0;
			}
			mu_log_count=0;
		} else if (pPacket[1]==0x04)  { // Polling for response
			int l_data_len=0;
			int l_position=(pPacket[2]<<24)|(pPacket[3]<<16)|(pPacket[4]<<8)|pPacket[5];
			memset(spi_data,0x00,1024);
			
			l_data_len = HTTPCli_readRawResponseBody_MSG_DONTWAIT(&motionupload_pHttpClient, spi_data+8, 1000);
			if ((mu_log_count%40)<3)
				UART_PRINT("MU: Response at %d len=%d value %s\r\n",motionupload_bodylen_current,l_data_len,spi_data+8);
			mu_log_count++;
			//if ((motionupload_bodylen_current==0) && (l_data_len>0))
			//	UART_PRINT("MU: Response header at %s\r\n",);
			//if ((l_data_len>0) || ((l_data_len==0)&&(motionupload_bodylen_current>0))){ // Get body
			if (l_data_len>=0){
				//l_data_len=0;
				l_data_len+=4;
				pPacket[0]=CMD_UPLOAD;
				pPacket[1]=0x06;
				pPacket[2]=(l_data_len>>8)&0xff;
				pPacket[3]=l_data_len&0xff;
				pPacket[4]=(motionupload_bodylen_current>>24)&0xff;;
				pPacket[5]=(motionupload_bodylen_current>>16)&0xff;;
				pPacket[6]=(motionupload_bodylen_current>>8)&0xff;
				pPacket[7]=(motionupload_bodylen_current)&0xff;
				set_down_anyka_cmd(pPacket,0);
			//	UART_PRINT("Response at %d len=%d\r\n",motionupload_bodylen_current,l_data_len);
				motionupload_bodylen_current+=(l_data_len-4);
			}
		} else if (pPacket[1]==0x07){ // close socket
			if (motionupload_available){
				myCinHttpClose(&motionupload_pHttpClient);
				motionupload_available=0;
				UART_PRINT("MU: close socket\r\n");
			}
		}
	} else if (packet_type==CMD_FILEUPLOAD){ // For file transferring
		spi_get_motion_file(pPacket);
	} else if (packet_type==RESPONES_AK_KL){
		UART_PRINT("AK:KL\r\n");
		AK_KeepAlive_status=1;
	}
    #ifdef SPI_CMD_11_12
    else {
        ret_val = spi_cmd_handler(spi_data
                        , spi_data_len - SPI_PACKET_HEADER_LEN
                        , packet_type, NULL, 0);
    }
    #endif

exit_error:
    return ret_val;
}

int recvbuf_get_spi_buff(char* l_data_buf){
	if (boot_mem[g_spi_recv_pos_read]!=0){
		memcpy(l_data_buf,boot_mem+g_spi_recv_pos_read,1024);
		boot_mem[g_spi_recv_pos_read]=0;
		g_spi_recv_pos_read=(g_spi_recv_pos_read+1024)%SPI_RECV_BUF_LEN;
		return 1;
	} else
		return -1;
}
int sendbuf_put_spi_buff(char* l_data_buf){
	if (spi_send_ptr[g_spi_send_pos_write]==0){
		memcpy(spi_send_ptr+g_spi_send_pos_write,l_data_buf,1024);
		g_spi_send_pos_write=(g_spi_send_pos_write+1024)%SPI_SEND_BUF_LEN;
		//UART_PRINT("g_spi_send_pos_write%d\r\n",g_spi_send_pos_write);
		return 1;
	} else
		return -1;
}
int true_bitrate=0;
int read_buffer_anyka(char l_response, int l_timeout)
{
	int ret_val = 0;
	struct u64_time begin_time, current_time;
	cc_rtc_get(&begin_time);
	
	for(;;)
	{
		//ret_val = RingBuffer_Read(ring_buffer_receive, data_buf, DMA_SIZE);
		
		// process received data buffer
		ret_val=recvbuf_get_spi_buff(data_buf);
			
		if (ret_val > 0)
		{
			if (data_buf[0] == RESPONSE_VERSION){
				memcpy(ak_version,data_buf+1,6);
				fw_version_format(ak_version_format,ak_version);
			}
			if(data_buf[0] == l_response)
			{
				if ((l_response!=CMD_AKOTA) || ((l_response==CMD_AKOTA) && (data_buf[1]==0x03))) {
					UART_PRINT("\r\nGET_TYPE OF ANYKA_SEND = 0x%X\r\n", data_buf[0]);
					if (l_response==RESPONSE_FTEST)
						true_bitrate=((int)data_buf[5]<<8)|data_buf[6];
					return data_buf[0];
				}
			}
		}
		cc_rtc_get(&current_time);
		if ((current_time.secs-begin_time.secs)>l_timeout){
			UART_PRINT("\r\nGET_TYPE OF ANYKA_SEND TIMEOUT\r\n");
			return -1;
		}
	}
}
int read_buffer_anyka_1(int l_position)
{
	int ret_val = 0;
	struct u64_time begin_time, current_time;
	int count=0;
	
	for(;;)
	{
		//ret_val = RingBuffer_Read(ring_buffer_receive, data_buf, DMA_SIZE);
		// process received data buffer
		ret_val=recvbuf_get_spi_buff(data_buf);
		if (ret_val > 0)
		{
			if ((data_buf[0] == CMD_AKOTA) && (data_buf[1] == 0x02) && (data_buf[2] == ((l_position>>24)&0xff)) && (data_buf[3] == ((l_position>>16)&0xff)) && (data_buf[4] == ((l_position>>8)&0xff)) && (data_buf[5] == ((l_position>>0)&0xff)))
			{
//				UART_PRINT("AK_OTA RESPONSE = 0x%X %d\r\n", data_buf[0], l_position);
				return l_position;
			}
		}
		osi_Sleep(7);
		count++;
//		UART_PRINT("%d>",count);
		if (count==(10*5)){
			UART_PRINT("Timeout receiving response at position %d\r\n",l_position);
			return -1;
		}
	}
}

int set_cds_spi(unsigned int l_cds);
extern int send_cds_request;
extern unsigned int adc_cds_read();
//*****************************************************************************
//
//! SPI Slave Interrupt handler
//!
//! This function is invoked when SPI slave has its receive register full or
//! transmit register empty.
//!
//! \return None.
//
//*****************************************************************************
extern int to_send_setting;
extern int play_dingdong_trigger;
extern int play_dingdong(void);
extern int wdt_run_status;
#ifdef NTP_CHINA
#define MAX_CYCLE_CDS (5000*1000)
#else
#define MAX_CYCLE_CDS (10000/4)
#endif
#define LIGHT_OFF_CDS 50
extern int sd_file_status;
extern int g_valprompt;
extern int low_bat_push;
unsigned int g_simeple_time=0; //ms/10
void spiSlaveHandleTask(void * param)
{
    int ret_val = 0;
	unsigned int l_cds_mV;

	init_buffer();
	stopwatch_start();

	for (;;)
	{
		while (spi_thread_pause)
			osi_Sleep(1000);
		if (send_cds_request!=-1){
			if (send_cds_request==0)
				send_cds_request=1;
			if ((send_cds_request%MAX_CYCLE_CDS)==CDS_DELAY){
				l_cds_mV=adc_cds_read();
				set_cds_spi(l_cds_mV);
		    }
			if ((send_cds_request%MAX_CYCLE_CDS)==(MAX_CYCLE_CDS-1)){
				set_cds_spi(LIGHT_OFF_CDS);
		    }
		    if (send_cds_request>0)
		    	send_cds_request++;
		}
		if (play_dingdong_trigger==DINGDONG_COUNT_MAX){
			if (low_bat_push>0)
				g_input_state=LED_STATE_CALL_LOWBAT;
			else
				g_input_state=LED_STATE_CALL_NORMALBAT;
			play_dingdong();
			play_dingdong_trigger=0;
		}
		if(g_valprompt != 0)
		{
			voiceprompt_play(g_valprompt);
			g_valprompt = 0;
		}
		if ((sd_file_status!=0)&&(send_cds_request>5)){
			spi_set_motion_file();
		}
        // process received data buffer
		ret_val=recvbuf_get_spi_buff(data_buf);
		
        //ret_val = RingBuffer_Read( ring_buffer_receive, data_buf, DMA_SIZE);
        if (ret_val > 0) {
            spi_data_processing(data_buf, DMA_SIZE, NULL, 0);
            g_simeple_time+=5;
            if (to_send_setting==1)
                to_send_setting=2;
        } else {
            if (ak_power_status){
                osi_Sleep(4);
                g_simeple_time+=40;
            } else {
                osi_Sleep(100); // Give more time for LPDS if AK is not yet powered
            }
        }
    }
}

// DkS: len must be mutiple of 1024 (DMA_SIZE)
int spi_send_data(uint8_t *data, int len) {
    int32_t ret_val = 0;
    int i = 0;
    #if (AK_SPI_TEST)
    // DkS: test data
    char test_data[1024] = {0};

    for (i = 0; i < 1024; i++) {
        test_data[i] = i;
    }
    data = test_data;
    len = sizeof(test_data);
    // DkS: end
    #endif // AK_SPI_TEST
 // copy data to buffer
	for (i = 0; i < 5; i++) {
    	//ret_val = RingBuffer_Write(ring_buffer_send, (char *)data, len);
    	ret_val = sendbuf_put_spi_buff(data);
	}
    // pull down ak read int pin
    ak_set_read_gpio(1);
	osi_Sleep(1);
	ak_set_read_gpio(0);
   	for (i = 0; i < 5; i++) {
    	//ret_val = RingBuffer_Write(ring_buffer_send, (char *)data, len);
    	ret_val = sendbuf_put_spi_buff(data);
	}
    // pull up ak read int pin
    osi_Sleep(10);
    //ak_set_read_gpio(0);
    return ret_val;
}
