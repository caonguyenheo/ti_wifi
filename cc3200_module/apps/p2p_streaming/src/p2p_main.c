
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "p2p_main.h"
#include "p2p.h"
#include "user_common.h"
// #include "simulator_frame.h"
#include "ntp.h"
#include "p2p_config.h"
#include "network_if.h"
#define OSI_STACK_SIZE          1024
#include "url_parser.h"
#include "mqtt_handler.h"
#include "p2p_packet.h"
#include "inet_pton.h"
#include "cc3200_aes.h"
#include "cc3200_system.h"
#include "date_time.h"
#include "ringbuffer.h"
#include "sleep_profile.h"
#include "state_machine.h"
#include "spi_slave_handler.h"
#include "system.h"
#include "network_handler.h"
#define P2P_USING_MUTEX         0
#if (P2P_USING_MUTEX)
#include <FreeRTOS.h>
#include <semphr.h>
static SemaphoreHandle_t socket_access;
#define SOCK_READ               0
#define SOCK_WRITE              1
static uint8_t g_using_socket = SOCK_WRITE;
#endif
#if (LPDS_MODE)
#define USE_MQTT_MODE           1
#else
#define USE_MQTT_MODE           0
#endif
#define DEBUG_ACK               1

extern OsiMsgQ_t g_MQTT_connection_flag;
extern OsiSyncObj_t g_NTP_init_done_flag;
extern OsiSyncObj_t g_userconfig_init;
extern OsiSyncObj_t g_start_streaming;
extern OsiSyncObj_t g_registered;
extern OsiSyncObj_t g_getNTP;

//extern RingBuffer* ring_buffer_send; 

extern int motionupload_available;
int g_valprompt = 0;

char p2p_key_char[P2P_KEY_CHAR_LEN] = {0};
char p2p_rn_char[P2P_RN_CHAR_LEN] = {0};
char p2p_key_hex[P2P_KEY_HEX_LEN] = {0};
char p2p_rn_hex[P2P_RN_HEX_LEN] = {0};

#define ACK_NUM 200
bool p2p_ps_ack[ACK_NUM] = {0};
uint32_t p2p_ps_start_pid = 0;
uint32_t p2p_ps_start_pid_ind = 0;

bool p2p_pr_ack[ACK_NUM] = {0};
uint32_t p2p_pr_start_pid = 0;
uint32_t p2p_pr_start_pid_ind = 0;

bool p2p_pl_ack[ACK_NUM] = {0};
uint32_t p2p_pl_start_pid = 0;
uint32_t p2p_pl_start_pid_ind = 0;



char ps_ip[P2P_PS_IP_LEN] = {0};
char ps_url[P2P_PS_URL_LEN] = {0};
char pr_ip[P2P_PS_IP_LEN] = {0};
char pl_ip[P2P_PS_IP_LEN] = {0};
char my_pr_ip[P2P_PS_IP_LEN] = {0};
char my_pl_ip[P2P_PS_IP_LEN] = {0};
uint32_t ps_port = 20000, pl_port = 20000, pr_port = 20000, my_pr_port = 20000, my_pl_port = 20000, ps_ssl_port = 443;
char str_ps_port[P2P_PS_PORT_LEN]={0};

#define MAX_CMD_BUFLEN 10

#define PS_TIMEOUT 7000
#define UPLOAD_TIMEOUT 5
#define AKKL_TIMEOUT 1
#define PR_TIMEOUT 7000
#define PL_TIMEOUT 7000
// Socket waiting time in ms
#define WAITING_TIME    15
#define PS_STREAMING_CONNECTING_TIMEOUT 6
#define PS_NOTSTREAM_CONNECTING_TIMEOUT_MARGIN 10
uint32_t ps_no_ack = PS_TIMEOUT;
uint32_t pr_no_ack = PR_TIMEOUT;
uint32_t pl_no_ack = PL_TIMEOUT;
uint32_t stream_no_maintain = 0;
int stream_found=0;
int32_t ps_socket = -1;
int32_t pl_socket = -1;
int32_t pr_socket = -1;

//#define RELAY_URL "relay-monitor.cinatic.com"
char relay_url_ch[rms_size];

//uint64_t utc_start=0;
int32_t ps_last_pid=-1;
int32_t pr_last_pid=-1;
int32_t pl_last_pid=-1;

typedef enum ps_state_e
{
    PS_OFF = 0,
    PS_INIT,
    PS_STREAMING,
    PS_CLOSING
} ps_state_e;

typedef enum pr_state_e
{
    PR_OFF = 0,
    PR_INIT,
    PR_STREAMING,
    PR_CLOSING
} pr_state_e;

typedef enum pl_state_e
{
    PL_OFF = 0,
    PL_INIT,
    PL_STREAMING,
    PL_CLOSING
} pl_state_e;

int32_t ps_state = PS_OFF;
int32_t pr_state = PR_OFF;
int32_t pl_state = PL_OFF;

#define RAW_VIDEO_BLOCK_LENGTH 999

typedef struct {
    char cmd_name[32];
    int cmd_id;
    bool has_agru; //back ward compatible, do not use
} cmd_table_st;

int cur_pid = 0;
/*#define MAX_FRAME_PKT_BUF 10
char frame_pkt_buf [MAX_FRAME_PKT_BUF][1019];
int frame_pkt_buf_len [MAX_FRAME_PKT_BUF];
int frame_pkt_buf_len_total;*/

void rev_process(void);
void p2p_ps_close(void);
void p2p_pr_close(void);
void p2p_pl_close(void);
void pid_reset(){
	cur_pid=0;
}

void dump_hex(char *data, int length) {
    int j;
    UART_PRINT("\n=====================================\n");
    for (j = 0; j < length; j++) {
        UART_PRINT("0x%x ", *(data + j));
    }
    UART_PRINT("\n=====================================\n");
}

// #define MAIN_LOOP_STATESLEEP 50
#define MAIN_LOOP_STATESLEEP 15
extern int connect_done;
//*****************************************************************************
//@brief Function to reboot system
//
//*****************************************************************************
extern int mqtt_connect_status;
int mqtt_max_waiting_time=0;
extern int ota_done;
int AK_KeepAlive_status=0;
int cmd_processing_status=0;
void p2p_main(void *pvParameters){
	long lRetVal = -1;
	int k=0, i;

    // sync with mqtt task
    // unsigned char uc_mqtt_connection = 0;
    // osi_MsgQRead(&g_MQTT_connection_flag, &uc_mqtt_connection, OSI_WAIT_FOREVER);
    // osi_MsgQDelete(&g_MQTT_connection_flag);

    // unsigned char uc_NTP_status = 0;
    // osi_MsgQRead(&g_NTP_init_done_flag, &uc_NTP_status, OSI_WAIT_FOREVER);
    // osi_MsgQWrite(&g_NTP_init_done_flag, &uc_NTP_status, OSI_NO_WAIT);

    //UART_PRINT("\r\nStart a P2P main section\r\n");
    #if (USE_MQTT_MODE)
    // wait for userconfig init done
    osi_SyncObjWait(&g_userconfig_init, OSI_WAIT_FOREVER);
    osi_SyncObjSignal(&g_userconfig_init);
	config_init();
    #endif

    // WAIT HERE UNTIL REGISTERED
    //osi_SyncObjWait(&g_registered, OSI_WAIT_FOREVER);
    //osi_SyncObjSignal(&g_registered);


#if (P2P_USING_MUTEX)
    vSemaphoreCreateBinary(socket_access);
#endif

    //newthread(mqtt)
    //
    // Start the frame_simulator task
    //
    // lRetVal = osi_TaskCreate(frame_simulator,
    //                 (const signed char *)"Frame simulator",
    //                 OSI_STACK_SIZE*3,
    //                 NULL,
    //                 11,
    //                 NULL );

    while ((mqtt_connect_status==0)||(get_ntp_status)){
	    if (ota_done == 1){
		    if (mqtt_max_waiting_time>90){
			    for(i=0;i<10;i++)
			    	UART_PRINT("MQTT wait 90s->reboot");
		    	system_reboot();
	    	}
	    } else {
		    if ((mqtt_max_waiting_time>(12*60)) && connect_done){
			    for(i=0;i<10;i++)
			    	UART_PRINT("MQTT wait 12mins->reboot");
		    	system_reboot();
	    	}
	    }
	    UART_PRINT("MQTT wait to connect");
	    osi_Sleep(1000);
	    mqtt_max_waiting_time++;
	    if (get_ntp_status)
			UART_PRINT("NTP....");
		if (pairing_in_progress)
			break;
	}
	if (pairing_in_progress==0){
		g_input_state=LED_STATE_NONE;
		signal_event(eEVENT_INIT_DONE); 
	}
	while(1){
        #if 1
		{	// Process PS request
            // UART_PRINT("1. ps_state=%d\r\n", ps_state);
			if (ps_state==PS_INIT){
				ps_state=PS_STREAMING;
				stream_no_maintain = 0;
				stream_found=0;
			}
            // UART_PRINT("2. ps_state=%d\r\n", ps_state);
			if (ps_state==PS_STREAMING){
				if (ps_no_ack>=PS_TIMEOUT){
					ps_state = PS_CLOSING;
					ps_no_ack=PS_TIMEOUT;
				}
				if (stream_found)
					i=PS_STREAMING_CONNECTING_TIMEOUT;
				else
					i=cam_settings[15]+PS_NOTSTREAM_CONNECTING_TIMEOUT_MARGIN;
				if (stream_no_maintain>=i){
					ps_state = PS_CLOSING;
					ps_no_ack=PS_TIMEOUT;
					UART_PRINT("stream_no_maintain %d",stream_no_maintain);
				}
				
			}
            // UART_PRINT("3. ps_state=%d\r\n", ps_state);
			if (ps_state == PS_CLOSING){
				p2p_ps_close();
				ps_state=PS_OFF;
			}
            // UART_PRINT("4. ps_state=%d\r\n", ps_state);
		}
		{	// Process pr request
			if (pr_state==PR_INIT){
				pr_state=PR_STREAMING;
			}
			if (pr_state==PR_STREAMING){
				if (pr_no_ack>=PR_TIMEOUT){
					pr_state = PR_CLOSING;
					pr_no_ack=PR_TIMEOUT;
				}
			}
			if (pr_state==PR_CLOSING){
				p2p_pr_close();
				pr_state=PR_OFF;
			}
		}
		{	// Process pl request
			if (pl_state==PL_INIT){
				pl_state=PL_STREAMING;
			}
			if (pl_state==PL_STREAMING){
				if (pl_no_ack>=PL_TIMEOUT){
					pl_state = PL_CLOSING;
					pl_no_ack=PL_TIMEOUT;
				}
			}
			if (pl_state==PL_CLOSING){
				p2p_pl_close();
				pl_state=PL_OFF;
			}
		}
		if ((ps_state==PS_OFF) && (pr_state==PR_OFF) && (pl_state==PL_OFF) && (cmd_processing_status==0)){
			if (pairing_in_progress==0){
				//UART_PRINT("off process");
				pid_reset();
				// enable_deep_sleep();
				if ((motionupload_available)&&(motionupload_available<(UPLOAD_TIMEOUT*1000/MAIN_LOOP_STATESLEEP))){
						if ((motionupload_available%(500/MAIN_LOOP_STATESLEEP))==0)
							UART_PRINT("MU: IP\r\n");
						motionupload_available++;
				} else if (get_ntp_status==0){
					if (AK_KeepAlive_status){
						osi_Sleep(AKKL_TIMEOUT*1000);
						AK_KeepAlive_status = 0;
					}
					signal_event(eEVENT_STOP_STREAMING);
					osi_SyncObjWait(&g_start_streaming, OSI_WAIT_FOREVER);
					osi_Sleep(200);
				} else if (get_ntp_status)
					UART_PRINT("NTP.");
			} else
				pid_reset();
		}
		#endif
		rev_process();
#ifndef NTP_CHINA
		if (pairing_in_progress==0)
			led_control(MAIN_LOOP_STATESLEEP);
#endif
		osi_Sleep(MAIN_LOOP_STATESLEEP);
	}
}
extern unsigned int yield_count;
extern unsigned int keepalive_count;
char sd_filename[64];
int sd_file_status=0;
int server_req_upgrade = 0;
int set_uploadinfo(char* l_data, int len);
int command_handler(char* input, int length, command_type commandtype, char* response, int* response_len) {
    int ret_val = 0;
    url_parser parsed_url;
    char* l_command_ptr = input;
    char l_response[400];
    char str_caminfo[256]={0};
    char *ptr_response = l_response;
    int l_response_len = 0;
    char str_command[64];
    int str_command_sz = sizeof(str_command);
    int command_num=-1;
    cmd_processing_status = 1;
    if (commandtype != MQTT_COMMAND && commandtype != P2P_COMMAND && commandtype != HTTP_COMMAND) {
        goto exit_error;
    }

    if (commandtype == MQTT_COMMAND) {
        if (input[0]!='{'){
            l_command_ptr = input + 1;
            yield_count = 0;
            keepalive_count = 0;
        } else {
           set_uploadinfo(input, length);
           cmd_processing_status = 0;
           return 200;
        }
    }

    ret_val = parse_url(l_command_ptr, &parsed_url);
    if (ret_val != 0) {
        // UART_PRINT("Parse failed!\r\n");
        goto exit_error;
    }

    if (commandtype == MQTT_COMMAND) {
        // UART_PRINT("MQTT command!\r\n");
        char str_mac[MAX_MAC_ADDR_SIZE] = {0};
        char str_time[20] = {0};

        get_mac_address(str_mac);
        ret_val = getValueFromKey(&parsed_url, STR_TIME, str_time, sizeof(str_time));
        if (ret_val != 0) {
            // UART_PRINT("Get time fail!\r\n");
            goto exit_error;
        }
        // MQTT command Prefix

        l_response_len += sprintf(ptr_response, "3id: %s&time: %s&", str_mac, str_time);
        // l_response_len += sprintf(ptr_response, "3id=%s&time=%s&", str_time);
        ptr_response += l_response_len;
        ret_val = getValueFromKey(&parsed_url, STR_REQ, str_command, str_command_sz);
        if (ret_val != 0) {
            // UART_PRINT("Get command fail! Error code: %d\r\n", ret_val);
            goto exit_error;
        }
    }
    else if (P2P_COMMAND == commandtype) {
        // UART_PRINT("P2P command!\r\n");
        ret_val = getValueFromKey(&parsed_url, STR_REQ, str_command, str_command_sz);
        if (ret_val != 0) {
            goto exit_error;
        }
    }
    else if (HTTP_COMMAND == commandtype) {
         // UART_PRINT("HTTP command!\r\n");
           ret_val = getValueFromKey(&parsed_url, STR_REQ, str_command, str_command_sz);
           if (ret_val != 0) {
               goto exit_error;
           }
       }
    else {
        goto exit_error;
    }
    signal_event(eEVENT_START_STREAMING);
    command_num=command_string_2_token(str_command);
    UART_PRINT("COMMAND HANDLER %d\r\n",command_num);
    sprintf(str_caminfo, "flicker=%d&flipup=%d&fliplr=%d&brate=%d&frate=%d&res=%d&svol=%d&mvol=%d&gop=%d&audduplex=%d&videoautorate=%d&agc=%d&callwait=%d&pwm=%d",
    cam_settings[0],
    cam_settings[1],
    cam_settings[2],
    ((int)cam_settings[3]<<8)|cam_settings[4],
    cam_settings[5],
    ((int)cam_settings[6]<<8)|cam_settings[7],
    cam_settings[9],
    cam_settings[10],
    cam_settings[11],
    cam_settings[12],
    cam_settings[13],
    cam_settings[14],
    cam_settings[15],
    cam_settings[16]);
    switch (command_num) {
    case CMD_GET_SESSION_KEY:
    {
        char str_mode[64] = {0};
        char str_streamname[128] = {0};
        char str_port[6] = {0};

        memset(str_mode, 0, sizeof(str_mode));
        memset(str_streamname, 0, sizeof(str_streamname));
        memset(str_port, 0, sizeof(str_port));

        ret_val = getValueFromKey(&parsed_url, STR_MODE, str_mode, sizeof(str_mode));
        if (ret_val != 0) {
            goto exit_error;
        }
        // ret_val = getValueFromKey(&parsed_url, STR_STREAM_NAME, str_streamname, str_streamname_sz);
        // if (ret_val != 0) {
        //     goto exit_error;
        // }

        // ret_val = getValueFromKey(&parsed_url, STR_PORT_1, str_port, str_port_sz);
        // if (ret_val != 0) {
        //     goto exit_error;
        // }

        if (strlen(STR_RELAY) == strlen(str_mode) && strcmp(STR_RELAY, str_mode) == 0) { //ex: "app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=relay&streamname=CC3A61D7DEC6_42627"
            if (PS_OFF == ps_state) {
                osi_SyncObjSignal(&g_getNTP);
                osi_SyncObjWait(&g_NTP_init_done_flag, OSI_WAIT_FOREVER);
                fast_ps_mode();
            }
            l_response_len += sprintf(ptr_response, "get_session_key: error=200,key=%s&sip=%s&sp=%u&rn=%s&%s", p2p_key_hex, ps_ip, ps_port, p2p_rn_hex, str_caminfo);
        }
        //ex: "app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=remote&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627"
        //ex: "app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627"
        if ((strlen(STR_REMOTE) == strlen(str_mode) && strcmp(STR_REMOTE, str_mode) == 0) || (strlen(STR_COMBINE) == strlen(str_mode) && strcmp(STR_COMBINE, str_mode) == 0)) {
            int l_enable_pr=0;
            char str_port_buf[STR_PORT_BUFFER_SZ];
            
            osi_SyncObjSignal(&g_getNTP);
            osi_SyncObjWait(&g_NTP_init_done_flag, OSI_WAIT_FOREVER);
            fast_ps_mode();
            
            if (pr_state == PR_OFF) {
	            UART_PRINT("pr_socket %d\r\n", pr_socket);
                pr_socket = new_pr_socket(my_pr_ip, &my_pr_port);
                if (pr_socket >= 0) {
	                // check return value
	                ret_val = getValueFromKey(&parsed_url, STR_IP, pr_ip, sizeof(pr_ip));
	                if (ret_val != 0) {
	                    goto exit_error;
	                }
	                ret_val = getValueFromKey(&parsed_url, STR_PORT_1, str_port_buf, sizeof(str_port_buf));
	                if (ret_val != 0) {
	                    goto exit_error;
	                }
	                pr_port = atoi(str_port_buf);
	                //UART_PRINT("%s\r\n", ptr_response);
	                PR_open(pr_socket);
	                pr_no_ack = 0;
	                pr_state = PR_INIT;
	                l_enable_pr=1;
            	}
            }
            //else  {
            //    l_response_len += sprintf(ptr_response, "get_session_key: error=-2");
            //}

            if (l_enable_pr)
            	l_response_len += sprintf(ptr_response, "get_session_key: error=200,port1=%d&ip=%s&key=%s&sip=%s&sp=%d&rn=%s&%s", my_pr_port, my_pr_ip, p2p_key_hex, ps_ip, ps_port, p2p_rn_hex, str_caminfo);
            else
            	l_response_len += sprintf(ptr_response, "get_session_key: error=200,port1=%d&ip=%s&key=%s&sip=%s&sp=%d&rn=%s&%s", -1 , "-3.0.1.0", p2p_key_hex, ps_ip, ps_port, p2p_rn_hex, str_caminfo);
            // l_response_len += sprintf(ptr_response, "%s=get_session_key&mode=%s&port1=%s&ip=%s&%s=%s", STR_REQ, STR_REMOTE, str_port, ps_ip, STR_STREAM_NAME, str_streamname);
        }
        /*
        if (strlen(STR_COMBINE) == strlen(str_mode) && strcmp(STR_COMBINE, str_mode) == 0) { //ex: "app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627"
            UART_PRINT("TESTPOINT COMBINE\n\r");
            if (ps_state == PS_OFF) {
                UART_PRINT("TESTPOINT COMBINE: PS_OFF\n\r");
                // FIXME:
                ps_socket = new_ps_socket();
                if (ps_socket < 0) {
                    goto exit_error;
                }
                PS_open(ps_socket);
                ps_state = PS_INIT;
                ps_no_ack = 0;
            }
            if (pr_state == PR_OFF) {
                UART_PRINT("TESTPOINT COMBINE: PR_OFF\n\r");
                char str_port_buf[STR_PORT_BUFFER_SZ];
                pr_socket = new_pr_socket(my_pr_ip, &my_pr_port);
                if (pr_socket < 0) {
                    goto exit_error;
                }
                // "get_session_key: error=200,port1=<my_pr_port>&ip=<my_pr_ip>&key=<p2p_key_hex>&sip=<ps_ip>&sp=<ps_port>&rn=<p2p_rn_hex>"
                l_response_len += sprintf(ptr_response, "get_session_key: error=200,port1=%u&ip=%s&key=%s&sip=%s&sp=%u&rn=%s", my_pr_port, my_pr_ip, p2p_key_hex, ps_ip, ps_port, p2p_rn_hex);
                // check return value
                ret_val = getValueFromKey(&parsed_url, STR_IP, pr_ip, sizeof(pr_ip));
                if (ret_val != 0) {
                    goto exit_error;
                }
                ret_val = getValueFromKey(&parsed_url, STR_PORT_1, str_port_buf, sizeof(str_port_buf));
                if (ret_val != 0) {
                    goto exit_error;
                }
                pr_port = atoi(str_port_buf);
                // FIXME:
                // PR_open(pr_socket);
                ps_no_ack = 0;
                pr_state = PR_INIT;
            }
            else {
                l_response_len += sprintf(ptr_response, "get_session_key: error=200,key=%s&sip=%s&sp=%u&rn=%s", p2p_key_hex, ps_ip, ps_port, p2p_rn_hex);
            }
            //signal_event(eEVENT_STREAMING);
        }*/
        if (strlen(STR_LOCAL) == strlen(str_mode) && strcmp(STR_LOCAL, str_mode) == 0) {//ex: "app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=local&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627"
            if (pl_state == PL_OFF) {
                char str_port_buf[STR_PORT_BUFFER_SZ];
                if (strlen(p2p_key_hex)==0)
                    memcpy(p2p_key_hex,"6866486c7173355467397461774f376a",32);
                if (strlen(p2p_rn_hex)==0)
                    memcpy(p2p_rn_hex,"647568717377664975426436",24);
                    
                pl_socket = new_pl_socket(my_pl_ip, &my_pl_port);
                // "get_session_key: error=200,port1=<my_pl_port>&ip=<my_pl_ip>&key=<p2p_key_hex>&rn=<p2p_rn_hex>"
                
                l_response_len += sprintf(ptr_response, "get_session_key: error=200,port1=%u&ip=%s&key=%s&rn=%s&%s" , my_pl_port, my_pl_ip, p2p_key_hex, p2p_rn_hex, str_caminfo);
                ret_val = getValueFromKey(&parsed_url, STR_IP, pl_ip, sizeof(pl_ip));
                if (ret_val != 0) {
                    goto exit_error;
                }
                ret_val = getValueFromKey(&parsed_url, STR_PORT_1, str_port_buf, sizeof(str_port_buf));
                if (ret_val != 0) {
                    goto exit_error;
                }
                pl_port = atoi(str_port_buf);
                pl_no_ack = 0;
                pl_state = PL_INIT;
                signal_event(eEVENT_STREAMING);
            }
            else {
                l_response_len += sprintf(ptr_response, "get_session_key: error=-2");
            }
        }

    }
    break;
    case CMD_GET_VERSION:
    {
        l_response_len += sprintf(ptr_response, "%s: %s", str_command, g_version_8char);
    }
    break;
    case CMD_ID_GET_MAC:
    {
    	char strMacAddr[13] = {0};
    	ret_val = get_mac_address(strMacAddr);
    	if (ret_val != 0) {
    	    goto exit_error;
    	}
    	l_response_len += sprintf(ptr_response, "%s: %s", str_command, strMacAddr);
    }
    break;
    case CMD_ID_GET_UDID:
    {
        char strUdid[32] = {0};
        ret_val = get_uid(strUdid);
        if (ret_val != 0) {
            goto exit_error;
        }
        l_response_len += sprintf(ptr_response, "%s: %s", str_command, strUdid);
    }
    break;
    case CMD_ID_CHECK_FIRMWARE_UPGRADE:
    {
        if ((ps_state == PS_OFF)&&(pr_state == PR_OFF)&&(pl_state == PL_OFF)){
            cam_settings[8]=UPGRADE_CMDRCV_HEAD;
            cam_settings_update();
            l_response_len += sprintf(ptr_response, "%s: 0", str_command);
            server_req_upgrade = 1;
        } else
            l_response_len += sprintf(ptr_response, "%s: -2", str_command);
    }
    break;
    case CMD_ID_RESTART_SYSTEM:
    {
          // optional restart system or switch device to station mode to connect to an AP
    	l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    	 *response_len = -1;
    	 memcpy(response, l_response, l_response_len);
    	 cmd_processing_status = 0;
    	 return -1;
    }
    break;
    case CMD_ID_GET_RT_LIST:
    {
    	l_response_len += sprintf(ptr_response, "%s: %s", str_command, "router_list");
    	*response_len = l_response_len;
    }
    break;
    case CMD_ID_SET_SERVER_AUTH:
    {
        char strValue[64] = {0}, strTimezone[32] = {0};

        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, strValue, sizeof(strValue));
        if (ret_val != 0){
            goto exit_error;
        }

        ret_val = getValueFromKey(&parsed_url, STR_CMD_TIMEZONE, strTimezone, sizeof(strTimezone));
        if (ret_val != 0){
           goto exit_error;
        }
        set_server_authen(strValue, strTimezone);
        UART_PRINT("API_key: %s TimeZone: %s\r\n", strValue, strTimezone);
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    }
    break;
    case CMD_ID_SETUP_WIRELESS_SAVE:
    {
       char str_auth[MAX_STR_PARAM_LEN], str_ssid[MAX_STR_PARAM_LEN], str_key[MAX_STR_PARAM_LEN], str_index[MAX_STR_PARAM_LEN];

       ret_val = getValueFromKey(&parsed_url, STR_CMD_AUTH, str_auth, sizeof(str_auth));
       if (ret_val != 0){
        	goto exit_error;
       }

       ret_val = getValueFromKey(&parsed_url, STR_CMD_SSID, str_ssid, sizeof(str_ssid));
       if (ret_val != 0){
          goto exit_error;
       }

       ret_val = getValueFromKey(&parsed_url, STR_CMD_KEY, str_key, sizeof(str_key));
       if (ret_val != 0){
         goto exit_error;
       }

       ret_val = getValueFromKey(&parsed_url, STR_CMD_INDEX, str_index, sizeof(str_index));
       if (ret_val != 0){
         goto exit_error;
       }
       UART_PRINT("\r\nssid=%s\r\n pass=%s\r\n auth=%s\r\n index=%s\r\n", str_ssid, str_key, str_auth, str_index);
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, set_wireless_config(str_ssid, str_key, str_auth));
    }
    break;
    /*
    case CMD_ID_GET_WIFI_CONNECTION_STATE:
    {
    	char state[20] = "wifi_connection";
        //get_wifi_connection_state(state);
    	l_response_len += sprintf(ptr_response, "%s: %s", str_command, state);
    }
    break;*/
    case CMD_ID_URL_SET:
    {
    	char str_api_url[MAX_STR_LEN ] = {0}, str_mqtt_url[MAX_STR_LEN ] = {0}, str_ntp_url[MAX_STR_LEN ] = {0}, str_rms_url[MAX_STR_LEN ] = {0},
    	     str_stun_url[MAX_STR_LEN ] = {0};

    	ret_val = getValueFromKey(&parsed_url, STR_CMD_API, str_api_url, sizeof(str_api_url));
    	if (ret_val != 0){
    		memset(str_api_url, 0x00, strlen(str_api_url));
    		strcpy(str_api_url, API_URL_DEFAULT);
    	}

    	ret_val = getValueFromKey(&parsed_url, STR_CMD_MQTT, str_mqtt_url, sizeof(str_mqtt_url));
    	if (ret_val != 0){
    		memset(str_mqtt_url, 0x00, strlen(str_mqtt_url));
    		strcpy(str_mqtt_url, MQTT_URL_DEFAULT);
    	}

    	ret_val = getValueFromKey(&parsed_url, STR_CMD_NTP, str_ntp_url, sizeof(str_ntp_url));
    	if (ret_val != 0){
    		memset(str_ntp_url, 0x00, strlen(str_ntp_url));
    		strcpy( str_ntp_url, NTP_URL_DEFAULT);
    	}

    	ret_val = getValueFromKey(&parsed_url, STR_CMD_RMS, str_rms_url, sizeof(str_rms_url));
    	if (ret_val != 0){
    		memset(str_rms_url, 0x00, strlen(str_rms_url));
    		strcpy(str_rms_url, RMS_URL_DEFAULT);
    	}

    	ret_val = getValueFromKey(&parsed_url, STR_CMD_STUN, str_stun_url, sizeof(str_stun_url));
    	if (ret_val != 0){
    		memset(str_stun_url, 0x00, strlen(str_stun_url));
    		strcpy(str_stun_url, STUN_URL_DEFAULT);
    	}
    	//UART_PRINT("api_url=%s\r\n mqtt_url=%s\r\n ntp_url=%s\r\n rms_url=%s\r\n stun_url=%s\r\n", str_api_url, str_mqtt_url, str_ntp_url, str_rms_url, str_stun_url);
    	l_response_len += sprintf(ptr_response, "%s: %d", str_command, set_url(str_api_url, str_mqtt_url, str_ntp_url, str_rms_url, str_stun_url));
    }
    break;
    /*
    case CMD_ID_URL_GET:
    {
    	char rece_api[api_size]={0}, rece_mqtt[mqtt_size]={0}, rece_ntp[ntp_size]={0}, rece_rms[rms_size]= {0},  rece_stun[stun_size]= {0};
    	get_url(rece_api, rece_mqtt, rece_ntp, rece_rms, rece_stun);
    	l_response_len += sprintf(ptr_response, "%s: api_url=%s&mqtt_url=%s&ntp_url=%s&rms_url=%s&stun_url=%s", str_command, rece_api, rece_mqtt, rece_ntp, rece_rms, rece_stun);
    }
    break;*/
    case CMD_ID_SET_FLICKER:
    {
  	  	char str_set_flicker[SIZE_FLICKER] = {0};
  		ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_flicker, sizeof(str_set_flicker));
  		if (ret_val != 0)
  		{
  			goto exit_error;
  		}
  		UART_PRINT("\r\nset_flicker_value = %s\r\n", str_set_flicker);
  		cam_settings[0]=atoi(str_set_flicker);
  		cam_settings_update();
  		l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
  		set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_FLICKER:
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[0]);
    break;*/
    case CMD_ID_SET_CITY_TIMEZONE:
    {
    	char str_set_timezone[32] = {0};
    	ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_timezone, sizeof(str_set_timezone));
    	if (ret_val != 0){
    		goto exit_error;
    	}
    	UART_PRINT("\r\nset_timezone_value = %s\r\n", str_set_timezone);
    	l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    }
    break;
    case CMD_ID_SET_VIDEO_FLIP:
    {
        char str_set_updown[SIZE_FLIP] = {0};
        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_updown, sizeof(str_set_updown));
        if (ret_val != 0){
            goto exit_error;
        }
        UART_PRINT("\r\nFlip UpDown value = %s\r\n", str_set_updown);
        cam_settings[1]=0;
        cam_settings[2]=atoi(str_set_updown);
        cam_settings_update();
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
        set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_FLIPUP:
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[1]);
    break;*/
    case CMD_ID_SET_BITRATE:
    {
     	char str_set_value_bitrate[SIZE_BITRATE] = {0};
     	int ret_val = 0, temp=0;

     	ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_value_bitrate, sizeof(str_set_value_bitrate));
     	if (ret_val != 0){
     		goto exit_error;
     	}
     	//UART_PRINT("\r\nstr_set_value_bitrate = %s\r\n", str_set_value_bitrate);
     	temp = atoi(str_set_value_bitrate);
     	cam_settings[3]=temp>>8;
     	cam_settings[4]=temp&0xff;
     	cam_settings_update();
    	l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    	set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_BITRATE:
    {
        int ret_val = 0, temp=0;
        temp=cam_settings[3]<<8;
        temp|=cam_settings[4];
        //UART_PRINT("get_bitrate: %d\r\n", temp);
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, temp);
    }
    break;*/
    case CMD_ID_SET_FRAMERATE:
    {
    	char str_set_framerate[SIZE_FRAMERATE] = {0};

       ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_framerate, sizeof(str_set_framerate));
       if (ret_val != 0){
    	   goto exit_error;
       }
       //UART_PRINT(" str_set_value_framerate = %s\r\n", str_set_framerate);
       cam_settings[5]=atoi(str_set_framerate);
       cam_settings_update();
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
       set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_FRAMERATE:
    {
       //UART_PRINT("get_framerate:  %d\r\n", cam_settings[5]);
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[5]);
    }
    break;*/
    case CMD_ID_SET_RESOLUTION:
    {

       char str_set_resolution[SIZE_RESOLUTION] = {0};
       int temp=0;

       ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_resolution, sizeof(str_set_resolution));
       if (ret_val != 0){
           goto exit_error;
       }
       //UART_PRINT("str_set_value_resolution = %s\r\n", str_set_resolution);
       temp = atoi(str_set_resolution);
       cam_settings[6]=temp>>8;
       cam_settings[7]=temp&0xff;
       cam_settings_update();
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
       set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_RESOLUTION:
    {
       int temp=0;
       temp=cam_settings[6]<<8;
       temp|=cam_settings[7];
       //UART_PRINT("get_resolution: %d\r\n", temp);
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, temp);
    }
    break;*/
    case CMD_ID_SET_GOP:
    {
    	char str_set_gop[4] = {0};

       ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_gop, sizeof(str_set_gop));
       if (ret_val != 0){
    	   goto exit_error;
       }
       cam_settings[11]=atoi(str_set_gop);
       cam_settings_update();
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
       set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_GOP:
    {
       //UART_PRINT("get_gop: %d\r\n", cam_settings[11]);
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[11]);
    }
    break;*/
    /*
    case CMD_ID_GET_BITRATELIMITOPT:
    {
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[13]);
    }
    break;*/
    case CMD_ID_SET_BITRATELIMITOPT:
    {
        char str_set_bitratelimitopt[4] = {0};
        
        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_set_bitratelimitopt, sizeof(str_set_bitratelimitopt));
        if (ret_val != 0){
           goto exit_error;
        }
        cam_settings[13]=atoi(str_set_bitratelimitopt);
        cam_settings_update();
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
        set_down_anyka(2);
    }
    break;
    /*
    case CMD_ID_GET_AGC:
    {
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[14]);
    }
    break;*/
    case CMD_ID_SET_AGC:
    {
        char str_agc[4] = {0};
        
        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_agc, sizeof(str_agc));
        if (ret_val != 0){
           goto exit_error;
        }
        cam_settings[14]=atoi(str_agc);
        cam_settings_update();
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
        set_down_anyka(2);
    }
    break;
    case CMD_ID_SET_CALLWAIT:
    {
        char str_callwait[4] = {0};
        
        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_callwait, sizeof(str_callwait));
        if (ret_val != 0){
           goto exit_error;
        }
        cam_settings[15]=atoi(str_callwait);
        cam_settings_update();
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    }
    break;
    /*
    case CMD_ID_GET_CALLWAIT:
    {
       l_response_len += sprintf(ptr_response, "%s: %d", str_command, cam_settings[15]);
    }
    break;*/
    case CMD_ID_SET_PWM:
    {
        char str_pwm[4] = {0};
        
        ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, str_pwm, sizeof(str_pwm));
        if (ret_val != 0){
           goto exit_error;
        }
        cam_settings[16]=atoi(str_pwm);
        cam_settings_update();
        l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
    }
    break;
    /*
    case CMD_GET_SPK_VOLUME:
        l_response_len += sprintf(ptr_response, "%s : -1", str_command);
    break;*/
#ifdef NTP_CHINA
    case CMD_ID_FILETRANSFER:
    {
		char file_mode[16], file_submode[16];
		int l_filewaitcount=0;
		ret_val = getValueFromKey(&parsed_url, "act", file_mode, sizeof(file_mode));
		UART_PRINT("\r\n\r\nFile xfer command: %d %d\r\n\r\n", ret_val, sd_file_status);
		if (ret_val != 0) {
			goto exit_error;
		}
		if (strlen("add") == strlen(file_mode) && strcmp("add", file_mode) == 0){
			memset(sd_filename,0x00,sizeof(sd_filename));
			ret_val = getValueFromKey(&parsed_url, "name", sd_filename, sizeof(sd_filename));
//			UART_PRINT("fileadd %d %s\r\n",ret_val,sd_filename);
			if (ret_val != 0) {
				goto exit_error;
			}
			fast_ps_mode();
			l_response_len += sprintf(ptr_response, "%s: 1", str_command);
			break;
		}
        
		if (strlen("start") == strlen(file_mode) && strcmp("start", file_mode) == 0){
			sd_file_status=1; // Send filename to AK
			ret_val = getValueFromKey(&parsed_url, "mode", file_submode, sizeof(file_submode));
			UART_PRINT("TESTPOINT FILE RELAY %d %s\r\n",ret_val,file_submode);
			l_filewaitcount = 0;
			while((sd_file_status<4)&&(l_filewaitcount<25)){ // Filename sending
				if (PS_OFF != ps_state){
					UART_PRINT("W%d",l_filewaitcount);
					osi_Sleep(200);
				}
				l_filewaitcount++;
			}
			if ((sd_file_status==4) && (strlen("relay") == strlen(file_submode) && strcmp("relay", file_submode) == 0)){
				sd_file_status = 4; // request AK to send file
				fast_ps_mode();
				/* error=200,port1=-1&ip=x.y.z.w
				* x: error code (remote socket fail (-1), same public IP (-2), full session (-3), mode not supported (-4), file not set or not available (-5))
				* y: number of PL client (0->2)
				* z: number of PR client (0->2)
				w: number of PS (0/1) */
				if (ps_socket>=0)
					l_response_len += sprintf(ptr_response, "p2p_file_xfer: error=200,key=%s&sip=%s&sp=%u&rn=%s", p2p_key_hex, ps_ip, ps_port, p2p_rn_hex);
				else
					l_response_len += sprintf(ptr_response, "p2p_file_xfer: error=200,port1=-1&ip=-1.0.0.0&key=31&sip=-1.0.0.0&sp=-1&rn=31");
				sd_file_status = 5; //file streaming
			} else{
				if (sd_file_status==4)
					l_response_len += sprintf(ptr_response, "p2p_file_xfer: error=200,port1=-1&ip=-4.0.0.0&key=31&sip=-4.0.0.0&sp=-1&rn=31");
				else
					l_response_len += sprintf(ptr_response, "p2p_file_xfer: error=200,port1=-1&ip=-5.0.0.0&key=31&sip=-5.0.0.0&sp=-1&rn=31");
			}
			break;
		}
		if (strlen("stop") == strlen(file_mode) && strcmp("stop", file_mode) == 0){
			if (sd_file_status>4){
				sd_file_status=7;
				l_filewaitcount=0;
				while((sd_file_status!=0)&&(l_filewaitcount<10)){ // Send filestop
					osi_Sleep(50);
					l_filewaitcount++;
				}
			}
			l_response_len += sprintf(ptr_response, "%s: 0", str_command);
			break;
		}
		break;
	}
#endif
	case CMD_ID_GET_WIFISTRENGTH:
		l_response_len += sprintf(ptr_response, "%s: %s:%d", str_command, ap_ssid, get_wifi_strength());
	break;
	case CMD_ID_GET_CAMINFO:
	{
#ifndef NTP_CHINA
		l_response_len += sprintf(ptr_response, "%s: %s&wifi=%d&bat1=%d&bat2=%d", str_command, str_caminfo, get_wifi_strength(), adc_bat1_read(), adc_bat2_read());
#else
		l_response_len += sprintf(ptr_response, "%s: %s&wifi=%d&bat1=%d", str_command, str_caminfo, get_wifi_strength(), adc_bat_read());
#endif
	}
	break;
	/*
	case CMD_ID_GET_VOLUME:
	{
		l_response_len += sprintf(ptr_response, "%s: spkvol=%d&micvol=%d", str_command, cam_settings[9], cam_settings[10]);
	}
	break;*/
	/*
	case CMD_ID_SET_VOLUME:
	{
		char spkvol[4]={0};
		char micvol[4]={0};
		int ret_val1, ret_val2;
		ret_val = getValueFromKey(&parsed_url, "spkvol", spkvol, sizeof(spkvol));
		ret_val |= getValueFromKey(&parsed_url, "micvol", micvol, sizeof(micvol));
		if (ret_val==0){
			cam_settings[9]=atoi(spkvol);
			cam_settings[10]=atoi(micvol);
			cam_settings_update();
			l_response_len += sprintf(ptr_response, "%s: 0", str_command);
		} else {
			l_response_len += sprintf(ptr_response, "%s: -1", str_command);
		}
		set_down_anyka(2);
	}
	break; */
	case CMD_ID_SET_PLAY_VOICE_PROMPT:
	{
		char voice_prompt[SIZE_VOICEPROMPT] = {0};
		ret_val = getValueFromKey(&parsed_url, STR_CMD_VALUE, voice_prompt, sizeof(voice_prompt));
		if (ret_val != 0)
		{
			goto exit_error;
		}
		UART_PRINT("\r\nvalue_receive_from_app = %s\r\n", voice_prompt);
		if(ps_state!=PS_OFF || pr_state!=PR_OFF || pl_state!=PL_OFF)
		{
			g_valprompt = atoi(voice_prompt);
			//UART_PRINT("\r\nvalue voice setting = %d\r\n", g_valprompt);
			l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
		}
		else
		{
			l_response_len += sprintf(ptr_response, "%s: %d", str_command, -1);
		}
	}
	break; 
	case CMD_STOP_SESSION:
		p2p_ps_close();
		p2p_pr_close();
		l_response_len += sprintf(ptr_response, "%s: %d", str_command, 0);
	break;
	case CMD_ID_FTEST:
	{
		char ak_raw_spi_pkg[8]={0};
		char button_led_str[2]={0};
		int button_led;
		char ir_led_str[2]={0};
		int ir_led;
		char tone_freq_str[6]={0};
		char loop_back_str[2]={0};
		int tone_freq;
		ak_raw_spi_pkg[0]=CMD_FTEST;
		g_input_state = LED_STATE_FTEST;
		ret_val = getValueFromKey(&parsed_url, "button_led", button_led_str, sizeof(button_led_str)); // 0:off, 1:on, NA: unchanged
		if (ret_val==0){
			button_led=atoi(button_led_str);
			if (button_led)
				ak_led1_set(1);
			else
				ak_led1_set(0);
		}
		ret_val = getValueFromKey(&parsed_url, "ir_led", ir_led_str, sizeof(ir_led_str)); // 0:off, 1:on, NA: unchanged
		if (ret_val==0){
			ir_led = atoi(ir_led_str);
			if (ir_led)
				ak_raw_spi_pkg[1]=1;
			else
				ak_raw_spi_pkg[1]=2;
		} else
			ak_raw_spi_pkg[1]=0;
		
		ret_val = getValueFromKey(&parsed_url, "tone_freq", tone_freq_str, sizeof(tone_freq_str));
		if (ret_val==0){
			tone_freq = atoi(tone_freq_str);
			if(tone_freq!=0){
				ak_raw_spi_pkg[2]=1;
				ak_raw_spi_pkg[3]=tone_freq>>8;
				ak_raw_spi_pkg[4]=tone_freq&0xFF;
			} else
				ak_raw_spi_pkg[2]=2;
		} else
			ak_raw_spi_pkg[2]=0;
			
		ret_val = getValueFromKey(&parsed_url, "loop_back", loop_back_str, sizeof(loop_back_str));
		if (ret_val==0){
			if (atoi(loop_back_str))
				ak_raw_spi_pkg[7]=1;
			else
				ak_raw_spi_pkg[7]=2;
		} else
			ak_raw_spi_pkg[7]=0;
		spi_thread_pause = 1;
		osi_Sleep(100);
		spi_write_raw(ak_raw_spi_pkg,8);
		spi_thread_pause = 0;
#ifndef NTP_CHINA
		l_response_len += sprintf(ptr_response, "%s: cds=%d&bat1=%d&bat2=%d&call_button=%d&true_bitrate=%d", str_command, adc_cds_read(),adc_bat1_read(),adc_bat2_read(),get_ring_pin_val(),true_bitrate);
#else
		l_response_len += sprintf(ptr_response, "%s: cds=%d&bat1=%d&call_button=%d&true_bitrate=%d", str_command, adc_cds_read(),adc_bat_read(),get_ring_pin_val(),true_bitrate);
#endif
	}
	break;
	default:
		UART_PRINT("\nUnknown command\n");
		if (HTTP_COMMAND == commandtype){
			cmd_processing_status = 0;
			return 404;
		}else
		 l_response_len += sprintf(ptr_response, "%s: -1", str_command);
		break;
	}
	// UART_PRINT("\nDEBUG: %s\n", l_response);

    if (commandtype == MQTT_COMMAND) {
        signal_event(eEVENT_STREAMING);
        char pub_topic[128] = {0};
        ret_val = getValueFromKey(&parsed_url, "app_topic_sub", pub_topic, sizeof(pub_topic));
        if (ret_val != 0) {
            UART_PRINT("No app topic\r\n");
            goto exit_error;
        }
        UART_PRINT("\r\nPUB: %s\r\n", l_response);
        if (server_req_upgrade){ //Delay
            while(server_req_upgrade<(10*1000*1000)){
                server_req_upgrade++;
            }
        }
        ret_val = mqtt_pub(pub_topic, l_response);
        if (ret_val < 0) {
            UART_PRINT("fail to pub\r\n");
            goto exit_error;
        }
        if (server_req_upgrade){ //Delay
            while(server_req_upgrade<(30*1000*1000)){
                server_req_upgrade++;
            }
            system_reboot();
        }
    }
    else {
        // Because response generated for MQTT so in case http command: remove "3" prefix
        if (response != NULL && response_len != NULL) {
        	  memcpy(response, l_response, l_response_len);
        	  UART_PRINT("Response: '%s'\r\n", response);
        }
        else {
            UART_PRINT("WARN: response/response_len are NULL\r\n");
        }
    }

    cmd_processing_status = 0;
    return 200;
exit_error:
    cmd_processing_status = 0;
    if (commandtype == MQTT_COMMAND){
        signal_event(eEVENT_STREAMING);
        l_response_len = -1;
        cmd_processing_status = 0;
        return 0;
    }
    else{
        l_response_len += sprintf(response, "%s: -1", str_command);
        //UART_PRINT("Error bad request %s\r\n",ptr_response);
        return 400;
    }
}

int stream_process(int l_ps_socket, char* l_ps_ip, int l_ps_port, uint32_t l_pid
    , uint32_t* l_p2p_ps_start_pid, uint32_t* l_p2p_ps_start_pid_ind
    , bool* l_p2p_ps_ack, char *input, int length) {
    int extra_length;
    int i;
    uint32_t l_pid_ind = 0;
    int ret_val;
    struct sockaddr_in dest_addr;
    struct in_addr dest_ip;
    // UART_PRINT("1");
    ret_val = inet_pton4(l_ps_ip, (unsigned char *)&dest_ip.s_addr);
    // return 0 means invalid
    if (ret_val == 0) {
        return -1;
    }
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = dest_ip.s_addr;
    dest_addr.sin_port = htons(l_ps_port);
    // UART_PRINT("2");

    #if 0
    if (l_pid<*l_p2p_ps_start_pid) {

    }
    else if (l_pid<=*l_p2p_ps_start_pid+ACK_NUM-1) {
        l_pid_ind = (l_pid-*l_p2p_ps_start_pid+*l_p2p_ps_start_pid_ind)%ACK_NUM;
        if (l_p2p_ps_ack[l_pid_ind]==false) {
            ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (ret_val < 0) {
                return -1;
            }
        }
    }
    else {
        extra_length = l_pid-(*l_p2p_ps_start_pid+ACK_NUM-1);
        ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (ret_val < 0) {
            return -1;
        }
        if (extra_length<ACK_NUM) {
            for(i=0; i < extra_length; i++) {
                l_p2p_ps_ack[(*l_p2p_ps_start_pid_ind+i)%ACK_NUM]=0;
            }
            *l_p2p_ps_start_pid_ind=(*l_p2p_ps_start_pid_ind+extra_length)%ACK_NUM;
            *l_p2p_ps_start_pid+=extra_length;
        }
        else {
            memset(l_p2p_ps_ack, 0, ACK_NUM);
            *l_p2p_ps_start_pid=l_pid;
            *l_p2p_ps_start_pid_ind=0;
        }
    }
    #else
    // UART_PRINT("socket=%d, len=%d\r\n", l_ps_socket, length);
#if (P2P_USING_MUTEX)
    if (xSemaphoreTake(socket_access, 20) != pdTRUE) {
        return -2;
    }
    if (g_using_socket != SOCK_WRITE) {
        g_using_socket = SOCK_WRITE;
        osi_Sleep(3);
    }
#endif
#if 0
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(l_ps_socket, &write_fds);
    if (select(l_ps_socket + 1, NULL, &write_fds, NULL, NULL) <= 0) {
        UART_PRINT("Why here???\r\n");
        return -3;
    }

    if (FD_ISSET(l_ps_socket, &write_fds) != 1) {
        UART_PRINT("You're not my choice\r\n");
        return -4;
    }
#endif
    // UART_PRINT("3");
    // UART_PRINT("SOC=%d, len=%d, addr=%u, port=%d\r\n", l_ps_socket, length, dest_addr.sin_addr.s_addr, dest_addr.sin_port);
    ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret_val <= 0) {
        UART_PRINT("error:%d\r\n", ret_val);
    }
    // ret_val = sendto(l_ps_socket, input, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    // if (ret_val <= 0) {
    //     UART_PRINT("error:%d\r\n", ret_val);
    // }
    // UART_PRINT("*");

#if (P2P_USING_MUTEX)
    xSemaphoreGive(socket_access);
#endif
    if (ret_val < 0) {
        return -1;
    }
    #endif
    // UART_PRINT("4");
    return 0;
}
static void print_buffer(uint8_t * buffer, _u32 bufLen)
{
    int i;
    UART_PRINT("\r\n");
for(i = 0; i < bufLen; i++)
{
  UART_PRINT("0x%02X,", buffer[i]);
}
    // UART_PRINT("%02X", buffer[0]);
    UART_PRINT("\n\r");
}
unsigned int l_log_pr_ps_count=0;
int gop_packet_index=0;
int packet_process(char *input, int length) {
    int32_t ret_val = 0;
    uint32_t l_pid=frame_2_pid(input);
    // UART_PRINT("l_pid:%d length:%d\n", l_pid, length);

     uint32_t block_pid = 0;
     block_pid  = (input[12] & 0xFF) << 24;
     block_pid |= (input[13] & 0xFF) << 16;
     block_pid |= (input[14] & 0xFF) << 8;
     block_pid |= (input[15] & 0xFF);
	if ((input[5]/10)==1){
		if ((input[5] == 10) && (block_pid==l_pid)){
			UART_PRINT("%d ",gop_packet_index);
			gop_packet_index = 0;
			// Temporary disable to compatible with app
			stream_no_maintain++;
		}else
			gop_packet_index++;
		
	}
#if 0
    uint32_t block_pid = 0;
    static uint32_t start_time = 0;
    uint32_t end_time = 0;
    static uint32_t block_len;
    static uint32_t block_counter = 0;
    static uint32_t pre_pid = 0;
    static uint32_t pre_block_pid = 0;

    block_pid  = (input[12] & 0xFF) << 24;
    block_pid |= (input[13] & 0xFF) << 16;
    block_pid |= (input[14] & 0xFF) << 8;
    block_pid |= (input[15] & 0xFF);
    // check validity
    // lack of pid
    if (l_pid - pre_pid != 1) {
        UART_PRINT("ERROR: block_PID=%u, lack packet=%u\n", block_pid, pre_pid + 1);
    }

    if (block_pid != pre_block_pid) {
        // get NEW block check if old block is full
        if (block_counter != 0) {
            UART_PRINT("ERROR: lack BLOCK=%u, block_len=%u but lack=%u\n", 
                pre_block_pid, block_len, block_counter);
        }
        end_time = stopwatch_get_ms();
        UART_PRINT("Time to send %u\n", end_time - start_time);
        start_time = stopwatch_get_ms();
        // GET block len
        block_len  = (input[10] & 0xFF) << 8;
        block_len  |= (input[11] & 0xFF);
        // UART_PRINT("INFO: BLOCK=%u, block_len=%u\n", block_pid, block_len);
        // reset block counter
        block_counter = block_len - 1;

        // print_buffer(input + 20, 16);

    } else {
        // Old block
        // count_down block counter
        block_counter--;
    }
    
    // Update new status
    pre_pid = l_pid;
    pre_block_pid = block_pid;
#endif
    int ps_send_count=0, pr_send_count=0, i;
    if ((input[5]/10)!=1) {// non-video
        ps_send_count = 1;
        pr_send_count = 1;
    }else { //video
        if (gop_packet_index<=120) {// send-video
            if ((ps_state==PS_STREAMING)&&(pr_state==PR_STREAMING)){
               if (gop_packet_index<=60)
                   ps_send_count = 1;
               else {
                   ps_send_count = 0;
               }
               if ((input[5]==10))
                   pr_send_count = 2;
               else
                   pr_send_count = 1;
            } else {
               if (((input[5]==10)) || (gop_packet_index<60)){
                   ps_send_count = 2;
                   pr_send_count = 2;
               } else {
                   ps_send_count = 1;
                   pr_send_count = 1;
               }
            }
        }
    }
        
    if (ps_state==PS_STREAMING) {
        if (l_log_pr_ps_count%80==0)
        	UART_PRINT("PS\r\n");
        for(i=0;i<ps_send_count;i++){
        	stream_process(ps_socket,ps_ip,ps_port,l_pid,&p2p_ps_start_pid,&p2p_ps_start_pid_ind,p2p_ps_ack,input,length);
    	}
    }
    if (pl_state==PL_STREAMING)  {
        if (l_log_pr_ps_count%80==0)
        	UART_PRINT("PL\r\n");
        stream_process(pl_socket,pl_ip,pl_port,l_pid,&p2p_pl_start_pid,&p2p_pl_start_pid_ind,p2p_pl_ack,input,length);
    }
    if (pr_state==PR_STREAMING) {
        if (l_log_pr_ps_count%80==0)
        	UART_PRINT("PR\r\n");
        for(i=0;i<pr_send_count;i++){
        	stream_process(pr_socket,pr_ip,pr_port,l_pid,&p2p_pr_start_pid,&p2p_pr_start_pid_ind,p2p_pr_ack,input,length);
        }
    }
    l_log_pr_ps_count++;
    return ret_val;
}

void p2p_ps_close()
{
    p2p_ps_start_pid = 0;
    p2p_ps_start_pid_ind = 0;
    ps_state = PS_OFF;
    ps_no_ack = PS_TIMEOUT;
    memset(p2p_ps_ack, 0, sizeof p2p_ps_ack);
    ps_last_pid = -1;
    if (ps_socket >= 0) {
        close(ps_socket);
        ps_socket = -1;
    }
}

void p2p_pl_close() 
{
    p2p_pl_start_pid = 0;
    p2p_pl_start_pid_ind = 0;
    pl_port = 20000;
    my_pl_port = 20000;
    pl_state = PL_OFF;
    pl_no_ack = PL_TIMEOUT;
    memset(pl_ip, 0, sizeof pl_ip);
    memset(my_pl_ip, 0, sizeof my_pl_ip);
    memset(p2p_pl_ack, 0, sizeof p2p_pl_ack);
    pl_last_pid = -1;
    if (pl_socket >= 0) {
        close(pl_socket);
        pl_socket = -1;
    }
}

void p2p_pr_close()
{
    p2p_pr_start_pid = 0;
    p2p_pr_start_pid_ind = 0;
    pr_port = 0;
    my_pl_port = 0;
    pr_state = PR_OFF;
    pr_no_ack = PR_TIMEOUT;
    memset(pr_ip, 0, sizeof pr_ip);
    memset(my_pr_ip, 0, sizeof my_pr_ip);
    memset(p2p_pr_ack, 0, sizeof p2p_pr_ack);
    pr_last_pid = -1;
    if (pr_socket >= 0) {
        close(pr_socket);
        pr_socket = -1;
    }
}
uint8_t expected_array[] = {0x80,0x80,0x4C,0x8B,0x88,0x17,0xF6,0x4C,0x19,0x26,0x1F,0xD3,0xF3,0xAF,0x5C,0xBC};
int check_packet(uint8_t *data, uint32_t len) {
    uint32_t i = 0;
    uint8_t *ptr_data = data;
    uint32_t PID = 0;
    uint32_t block_PID = 0;
    uint32_t real_idx = 0;
    uint32_t sz_expected_array = sizeof(expected_array);
    if ( ptr_data == NULL) {
        return -1;
    }
    
    PID = (ptr_data[19] & 0xFF) << 24;
    PID |= (ptr_data[20] & 0xFF) << 16;
    PID |= (ptr_data[21] & 0xFF) <<  8;
    PID |= (ptr_data[22] & 0xFF) <<  0;

    block_PID = (ptr_data[15] & 0xFF) << 24;
    block_PID |= (ptr_data[16] & 0xFF) << 16;
    block_PID |= (ptr_data[17] & 0xFF) <<  8;
    block_PID |= (ptr_data[18] & 0xFF) <<  0;

    UART_PRINT("Block_PID=%u, PID=%u\r\n", block_PID, PID);
    real_idx = (PID - block_PID)*999;

    ptr_data = data + 23; // 3 spi header + 20 p2p header
    for (i = 0; i < len - 23; i++) {
        if (ptr_data[i] != expected_array[real_idx%sz_expected_array]) {
            UART_PRINT("idx=%u, real_idx=%u, DATA=%x, EXPECTED=%x\r\n", i, real_idx, ptr_data[i], expected_array[real_idx%sz_expected_array]);
            return -2;
        }
        real_idx++;
    }
    return 0;
}
#define DEBUG_TALK_BACK         0
#if (DEBUG_TALK_BACK)
#define parse_frame(x, y)      parse_talkback_frame((x), (y)) 
void parse_talkback_frame(uint8_t *data, int32_t length) {
    int32_t l_start_pid = 0;
    int32_t l_block_len = 0;
    int32_t l_pid = 0;
    int32_t l_time_stamp = 0;
    uint8_t *ptr_data = NULL;

    ptr_data = data;
    l_start_pid     = ntohl(*(int32_t *)(ptr_data + 12));
    l_block_len     = ntohs(*(int16_t *)(ptr_data + 10));
    l_pid           = ntohl(*(int32_t *)(ptr_data + 16));
    l_time_stamp    = ntohl(*(int32_t *)(ptr_data + 6));
    // UART_PRINT("parse_talkback_frame\r\n");
    UART_PRINT("Rev Audio, start_pid=%d, block_len=%d, pid=%d, time_stamp=%u, length=%d\r\n"
                        , l_start_pid
                        , l_block_len
                        , l_pid
                        , l_time_stamp
                        , length - 20);
}
#else
#define parse_frame(x, y)
#endif
extern void print_buffer(uint8_t * buffer, _u32 bufLen);

int32_t spi_add_checksum(char *p_data_send)
{
	int32_t i32Ret = 0;
	unsigned short usCsCalc = 0;
	unsigned char acChecksumErr[2] = {0x00, 0x00};
	int iterator = 0, i = 0;

	if(p_data_send == NULL)
	{
		i32Ret = -1;
	}
	else
	{
		//for(i = 0; i < 1024; i ++)
		//{
		//	*(p_data_send + i) = i%255;
		//}
		/* Add checksum error */
		for(iterator = 0; iterator < (1024-2); iterator++)
		{
			usCsCalc += *(p_data_send + iterator);
		}
		*(p_data_send + 1024-2) = (char)((usCsCalc >> 8) & 0xFF);
		*(p_data_send + 1024-1) = (char)(usCsCalc & 0xFF);
	}
	return i32Ret;
}
/*
void check_pakage_lost(int length, char *data)
{


    uint32_t ack_pid = ack_2_pid_ack(data);
    uint16_t payload_len = 0;
    uint16_t total_bit = 0;
    char * ptr_payload = NULL;

    // UART_PRINT("\r\nACK=%d\r\n", ack_pid);
    ptr_payload = data + 10; // refer frame format
    payload_len = length - (ptr_payload - data);

    {
        #define UP_PATTERN      "01"
        #define DONW_PATTERN    "10"
        uint32_t len = 0;
        uint32_t i = 0;
        len = payload_len;
        char ack_pattern[512] = {0};
        char output[256] = {0};
        uint16_t output_len = 0;

        uint16_t ack_pattern_sz = sizeof(ack_pattern);
        char *ptr_pattern;
        char *ptr_next_pattern = NULL;
        ptr_pattern = (char *)ack_pattern;

        memset(ack_pattern, 0x00, sizeof(ack_pattern));
        for (i = 0; i < len; i++) {
            ptr_pattern += snprintf(ptr_pattern
            // ptr_pattern += sprintf(ptr_pattern
                , ack_pattern_sz - (ptr_pattern - (char *)ack_pattern)
                , "%x"
                , ptr_payload[i]);
        }

        // UART_PRINT("%s\r\n", ack_pattern);

        // Analize
        memset(output, 0x00, sizeof(output));

        ptr_pattern = (char *)ack_pattern;
        if (*ptr_pattern == '0') {
            ptr_next_pattern = UP_PATTERN;
        }
        else {
            ptr_next_pattern = DONW_PATTERN;
        }

        while (1) {
            ptr_pattern = strstr(ptr_pattern, ptr_next_pattern);
            if (ptr_pattern == NULL) {
                break;
            }

            if (UP_PATTERN == ptr_next_pattern) {
                // PRINT and process
                #if (DEBUG_ACK)
                output_len += snprintf(output + output_len
                    , sizeof(output) - output_len
                    , "^%d^"
                    , (ptr_pattern - ack_pattern));
                    // , ack_pid + (ptr_pattern - ack_pattern));
                #endif
                // NEXT
                ptr_next_pattern = DONW_PATTERN;
            }
            else {
                #if (DEBUG_ACK)
                output_len += snprintf(output + output_len
                    , sizeof(output) - output_len
                    , "_%d_"
                    , (ptr_pattern - ack_pattern));
                #endif
                    // , ack_pid + (ptr_pattern - ack_pattern));
                // PRINT and process
                ptr_next_pattern = UP_PATTERN;
            }

            // FIND NEXT OCCURRED
            ptr_pattern += 1;
        }

        if (output_len < 2) {
            #if (DEBUG_ACK)
            UART_PRINT("PID: %d - %s\r\n", ack_pid, ack_pattern);
            #endif
        }
        else {
            #if (DEBUG_ACK)
            UART_PRINT("%s\r\n", output);
            #endif
        }
    }

}*/
int to_send_setting=0;
int talkback_count=0;
static int print_count=0;
static int g_current_talkback_pid=-1;
//int count_to_measure = 0;
void
rev_process_one(int l_ps_socket, char* l_ps_ip, int l_ps_port, uint32_t* l_p2p_ps_start_pid, 
                uint32_t* l_p2p_ps_start_pid_ind, bool* l_p2p_ps_ack, 
                uint32_t* l_ps_no_ack, int32_t* last_cmd_pid) 
{

    struct sockaddr_in dest_addr;
    _u32 uiDestIp;
    char data[1100]={0}, response[1024] = {0};
    int l_pid;
    uint32_t l_pid_ind = 0;
    int length, temp_len, response_len;
    char iv[16]={0};
    char temp_buf[512] = {0}; // For Command Only
    size_t oSize;
    long lRetVal = -1;
    int i;
    int media_type;

    #if 0 
    struct timeval timeout;
    timeout.tv_sec = WAITING_TIME;
    timeout.tv_usec = 0;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(l_ps_socket, &read_fds);

    // do {

    
    UART_PRINT("x");
    lRetVal = select(l_ps_socket + 1, &read_fds, NULL, NULL, &timeout);
    if (lRetVal <= 0) {
        // FIXME: close socket
        if (lRetVal == 0) {
            // end session
            // enable_deep_sleep();
            (*l_ps_no_ack)+=MAIN_LOOP_STATESLEEP + WAITING_TIME*1000;
            UART_PRINT("OMG___TIMEOUT=%d\r\n", *l_ps_no_ack);
        }
        return;
    }
    if (FD_ISSET(l_ps_socket, &read_fds) != 1) {
        UART_PRINT("something wrong, %d\r\n", l_ps_socket);
        return;
    }
    #else

#if (P2P_USING_MUTEX)
    if (xSemaphoreTake(socket_access, 10) != pdTRUE) {
        return;
    }
    if (g_using_socket != SOCK_READ) {
        g_using_socket = SOCK_READ;
        osi_Sleep(3);
    }
    else {
        // osi_Sleep(3);
    }
#endif

    // UART_PRINT("y");
    // length = recv(l_ps_socket, data, sizeof(data), MSG_DONTWAIT);
    // UART_PRINT("z");
    // wait for 15s
    length = recv(l_ps_socket, data, sizeof(data), MSG_DONTWAIT);
    #endif

#if (P2P_USING_MUTEX)
    xSemaphoreGive(socket_access);
#endif
    if (to_send_setting==2)
    {
		set_down_anyka(1); //Send NTP
		to_send_setting=3;
		// Make use of this variable
#ifndef NTP_CHINA
		//UART_PRINT("Battery1:%d\r\n",adc_bat1_read());
		//UART_PRINT("Battery2:%d\r\n",adc_bat2_read());
		/*
		if ((count_to_measure%10)==1){
			adc_bat1_read();
			adc_bat2_read();
		}
		count_to_measure++;*/

#else
		UART_PRINT("Battery:%d\r\n",adc_bat_read());
#endif
	}

	if ((length >= 10) && (length<=1019)) {
		media_type = data_2_mediatype(data);
		if ((media_type/10 == eMediaTypeACK) && (length > 10))
		{
			//UART_PRINT("Recv%d\r\n",media_type);
			if (media_type==eMediaSubTypeMediaFileACK){
				response[0] = FILE_ACK;
			}else
				response[0] = DATA_ACK;
			/*
			if ((print_count%20)==0){ // File ACK
				UART_PRINT("ACK:");
				for(i=0;i<10;i++)
					UART_PRINT("%02x ",data[i+5]);
			}*/
			print_count++;
			if ((print_count%2)==0){
				response[1] = ((length - 6) >> 8) & 0xFF; // minus 6 byte First (AUTH (4 byte) Reserv (1 byte) Media Type (1 byte) )
				response[2] = (length - 6) & 0xFF;
				memcpy(response + 3, data + 6, (length - 6));
				spi_add_checksum(response);
				//RingBuffer_Write(ring_buffer_send, (char *)response, 1024);
				sendbuf_put_spi_buff(response, 1024);
			}
			if (to_send_setting==3)
			{
				//UART_PRINT("to_send_setting==3\r\n");
				set_down_anyka(2); //Send setting
				to_send_setting=0;
			}
//			check_pakage_lost(length, data);
        }
        else if (media_type/10 == eMediaTypeTalkback) {
            static uint32_t start_time = 0;
            static uint32_t curr_time = 0;
            uint32_t delta_time = 0;
            static uint32_t recv_len = 0;
            l_pid = frame_2_pid(data);
            //UART_PRINT("P%d-%d\r\n", l_pid, length);
            if ((l_pid<=g_current_talkback_pid)&&(l_pid>=(g_current_talkback_pid-16))){
	            //UART_PRINT("NoD %d NeD %d\r\n", g_current_talkback_pid, l_pid);
                return;
            }
            g_current_talkback_pid = l_pid;
            parse_frame(data, length);
            memset(response, 0x00, sizeof(response));
            // UART_PRINT("y");
            // UART_PRINT("TALK BACK PID=%u\r\n", l_pid);

		// Trung: disable count data. It may be cause SPI lost data
		#if 0
            if (start_time == 0) {
                start_time = stopwatch_get_ms();
            }
            curr_time = stopwatch_get_ms();
            delta_time = curr_time - start_time;
            if (delta_time >= 1000) {
                UART_PRINT("RECV Bytes/s = %u\r\n", recv_len);

                recv_len = 0;
                start_time = curr_time;
            }

            recv_len += length - 20;
		#endif

            // Set spi packet header
            response[0] = 0x03;
            response[1] = (length >> 8) & 0xFF;
            response[2] = length & 0xFF;	
            memcpy(response + 3, data, length);
			// Trung: add checksum error
			spi_add_checksum(response); 

            /*UART_PRINT("Ringbuf avai=%d\r\n", RingBuffer_GetWriteAvailable(ring_buffer_send));*/
            /*print_buffer(response, length);*/
/*            if (check_packet(response, length) < 0) {*/
                /*print_buffer(response, 1024);*/
            /*}*/
            //if ((talkback_count%TALKBACK_DROP_PERIOD)!=(TALKBACK_DROP_PERIOD-1))
            //if ((l_pid%2)==1)
                //RingBuffer_Write(ring_buffer_send, (char *)response, 1024);
                sendbuf_put_spi_buff(response);
            talkback_count++;
            // UART_PRINT("z");
        }
        else if ((media_type / 10 == eMediaTypeCommand) && (length<=512)) {
            stream_no_maintain = 0;
            stream_found=1;
            UART_PRINT("CMD\r\n");
            /*
            lRetVal = inet_pton4(l_ps_ip, (unsigned char *)&uiDestIp);
            // return 0 means invalid
            if (lRetVal == 0) {
                UART_PRINT("rev_process_one: INVALID IP\n");
                return;
            }
            memset((char *)&dest_addr, 0, sizeof(dest_addr));
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_addr.s_addr = uiDestIp;
            dest_addr.sin_port = htons(l_ps_port);*/
            /*if (0) {
                l_pid = command_2_pid(data);
                // UART_PRINT("RECV: COMMAND PID=%d, len=%d\n", l_pid, length);
                if (((int32_t)l_pid) > *last_cmd_pid) {
                    int dec_pid = 0;
                    char *ptr_command = NULL;
                    // UART_PRINT("RECV: raw=%s\n", data);
                    // dump_hex(data, length);
                    aes_decrypt_cbc(p2p_key_char, iv, data + 10, temp_buf, ((length - 10) / 16) * 16, &oSize);
                    // UART_PRINT("RECV: size=%d\n", oSize);
                    dump_hex(temp_buf, oSize);
                    // NOTE: PID is include in payload, get PID and compare with frame PID
                    // Endian: little vs big
                    dec_pid |= (0xFF&temp_buf[0]) << 24;
                    dec_pid |= (0xFF&temp_buf[1]) << 16;
                    dec_pid |= (0xFF&temp_buf[2]) << 8;
                    dec_pid |= (0xFF&temp_buf[3]) << 0;
                    // If someone try to send fake command
                    if (dec_pid != l_pid) {
                        UART_PRINT("INVALID: dec_pid=%d, l_pid=%d\n", dec_pid, l_pid);
                        return;
                    }
                    ptr_command = temp_buf + 4; // first 4 bytes for PID
                    // UART_PRINT("RECV: command=%s\n", ptr_command);
                    command_handler(ptr_command, oSize, P2P_COMMAND, response + 12, &response_len);
                    // feed header
                    if (response_len > 0) {
                        memcpy(response, data, 10);
                        temp_len = (response_len/16)*16;
                        if (temp_len != response_len) {
                            temp_len += 16;
                        }
                        response[5] = eMediaSubTypeCommandResponse;
                        response[10] = (temp_len>>8)&0xFF;
                        response[11] = temp_len&0xFF;
                        temp_len += 12;
                        aes_encrypt_cbc(p2p_key_char, iv, response+12, response + 12, temp_len, &oSize);
                        sendto(l_ps_socket, response, oSize + 12, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    }
                    *last_cmd_pid = l_pid;
                }
            }*/
        } 
        *l_ps_no_ack = 0;
        if ((print_count%4)==0)
            UART_PRINT("x");
        // UART_PRINT("recv length %d ", length);
        // UART_PRINT("x");
        /*for(i=0;i<336;i++)
        	UART_PRINT("%d ", data[10+i]);*/
    }
    else {
	    // UART_PRINT("No recv "); 
        (*l_ps_no_ack)+=MAIN_LOOP_STATESLEEP;
    }
    // } while(length > 0);
}

void 
rev_process(void)
{
    if (PS_STREAMING == ps_state) {
        rev_process_one(ps_socket, ps_ip, ps_port, &p2p_ps_start_pid, &p2p_ps_start_pid_ind, p2p_ps_ack, &ps_no_ack, &ps_last_pid);
    }
    if (PL_STREAMING == pl_state) {
        rev_process_one(pl_socket, pl_ip, pl_port, &p2p_pl_start_pid, &p2p_pl_start_pid_ind, p2p_pl_ack, &pl_no_ack, &pl_last_pid);
    }
    if (PR_STREAMING == pr_state) {
        rev_process_one(pr_socket, pr_ip, pr_port, &p2p_pr_start_pid, &p2p_pr_start_pid_ind, p2p_pr_ack, &pr_no_ack, &pr_last_pid);
    }
}

int32_t get_ps_info(char *out_buf, uint32_t out_buf_sz, uint32_t *used_bytes) {
    // init p2p
    #if (LPDS_MODE == 0)
    osi_SyncObjWait(&g_userconfig_init, OSI_WAIT_FOREVER);
    #endif
    config_init();
    *used_bytes += snprintf(out_buf, out_buf_sz, "&key=%s&sip=%s&sp=%u&rn=%s", p2p_key_hex, ps_ip, ps_port, p2p_rn_hex);
    return 0;
}
extern uint32_t g_lasttimeget;
int32_t mqtt_get_ps_info(char *out_buf, uint32_t out_buf_sz, uint32_t *used_bytes) {
    config_init();
    memset(out_buf,0x00,out_buf_sz);
    *used_bytes += snprintf(out_buf, out_buf_sz, "{\"type\":1,\"cmd\":\"add_event\",\"is_rp\":true,\"cmd_id\":%d,\"cli_id\":\"%s\",\"evt\":6,\"st\":{\"key\":\"%s\",\"ip\":\"%s\",\"p\":\"%d\",\"rn\":\"%s\"}}", g_lasttimeget,g_mycinUser.device_udid, p2p_key_hex, ps_ip, ps_port, p2p_rn_hex);
    return 0;
}

/*
int32_t event_body_json(char *out_buf, uint32_t out_buf_sz, uint32_t *used_bytes, char* l_udid, char* l_token) {
    // init p2p
    #if (LPDS_MODE == 0)
    osi_SyncObjWait(&g_userconfig_init, OSI_WAIT_FOREVER);
    #endif
    config_init();
    *used_bytes += snprintf(out_buf, out_buf_sz, "{\"device_id\":\"%s\",\"event_type\":1,\"st_data\":{\"key\":\"%s\",\"rn\":\"%s\",\"sip\":\"%s\",\"sp\":%u},\"device_token\":\"%s\",\"is_ssl\":false}",l_udid, p2p_key_hex, p2p_rn_hex, ps_ip, ps_port, l_token);
    return 0;
}*/
int32_t spi_cmd_ps_info(char *out_buf) {
	int len1=strlen(p2p_key_char);
	int len2=strlen(p2p_rn_char);
	int len3=strlen(ps_ip);
	int len4=strlen(str_ps_port);
	int pos1=0;
	int pos2=1+len1;
	int pos3=2+len1+len2;
	int pos4=3+len1+len2+len3;
	int i=0;
	
	out_buf[pos1]=len1;
	out_buf[pos2]=len2;
	out_buf[pos3]=len3;
	out_buf[pos4]=len4;
	memcpy(out_buf+pos1+1,p2p_key_char,len1);
	memcpy(out_buf+pos2+1,p2p_rn_char,len2);
	memcpy(out_buf+pos3+1,ps_ip,len3);
	memcpy(out_buf+pos4+1,str_ps_port,len4);
	/*
	UART_PRINT("\r\n");
	for(i=0;i<(4+len1+len2+len3+len4);i++)
		UART_PRINT("%02x ", out_buf[i]);
	UART_PRINT("\r\n");*/
}


int32_t fast_ps_mode() {
    UART_PRINT("START PS STREAM\r\n");
    if (PS_OFF == ps_state) {
        ps_socket = new_ps_socket();
        if (ps_socket < 0) {
        goto exit_error;
        }
        #if (LPDS_MODE == 0)
        // Make sure NTP is ready
        osi_SyncObjWait(&g_NTP_init_done_flag, OSI_WAIT_FOREVER);
        #endif

        PS_open(ps_socket);
        ps_no_ack = 0;
        ps_state = PS_INIT;
        signal_event(eEVENT_STREAMING);
        ps_no_ack = 0;
    }
    else {
        if (ps_socket < 0) {
            goto exit_error;
        }
        #if (LPDS_MODE == 0)
        // Make sure NTP is ready
        osi_SyncObjWait(&g_NTP_init_done_flag, OSI_WAIT_FOREVER);
        #endif

        PS_open(ps_socket);
        //ps_no_ack = 0;
        //ps_state = PS_INIT;
        //signal_event(eEVENT_STREAMING);
        ps_no_ack = 0;
    }
        
    return 0;
exit_error:
    return -1;

}
// STOP STREAM, send a command to notify master board
//  and wait for command to get into hibernate mode
int32_t stop_stream_process(void) {

    // Send spi cmd to notify streaming is over
    // retry for xxx times or wait for response
    // Wait for upload done msg to get into hibernate mode
    return 0;
}

#if 0
void
frame_process(char* input, int length, char mediatype){
    int i,j;
    uint32_t block_num = 0;
    char packet[1200] = {0};
    int enc_pos = 0;
    uint32_t timestamp_ms;
    i = 0;
    while((length-i) >= 16) {
        aes_encrypt_ecb(p2p_key_char, input+i);
        i += (1024+16);
    }
    block_num = (length/RAW_VIDEO_BLOCK_LENGTH);
    if (block_num*RAW_VIDEO_BLOCK_LENGTH != length) {
        block_num++;
    }
    timestamp_ms = stopwatch_get_ms();
    packet[0]='V';
    packet[1]='L';
    packet[2]='V';
    packet[3]='L';
    packet[5]=mediatype;
    packet[6]=timestamp_ms >>24;
    packet[7]=(timestamp_ms >>16)&0xFF;
    packet[8]=(timestamp_ms >>8)&0xFF;
    packet[9]=timestamp_ms&0xFF;
    packet[10]=block_num>>8;
    packet[11]=block_num&0xFF;
    packet[12]=(cur_pid>>24);
    packet[13]=(cur_pid>>16)&0xFF;
    packet[14]=(cur_pid>>8)&0xFF;
    packet[15]=cur_pid&0xFF;
    //Cheat only, To be improved
    /*for(i = 0; i < frame_pkt_buf_len_total; i++) {
        packet_process(&frame_pkt_buf[i][0], frame_pkt_buf_len[i]);
    }
    frame_pkt_buf_len_total = 0;*/
    
    i=0;
    while(1){
        packet[16]=((cur_pid+i)>>24);
        packet[17]=((cur_pid+i)>>16)&0xFF;
        packet[18]=((cur_pid+i)>>8)&0xFF;
        packet[19]=(cur_pid+i)&0xFF;
        if (length >= RAW_VIDEO_BLOCK_LENGTH) {
            memcpy(packet+20, input, RAW_VIDEO_BLOCK_LENGTH);
            /*if (i < MAX_FRAME_PKT_BUF) {
                memcpy(&frame_pkt_buf[i][0], packet, 20+RAW_VIDEO_BLOCK_LENGTH);
                frame_pkt_buf_len[i] = 20 + RAW_VIDEO_BLOCK_LENGTH;
                frame_pkt_buf_len_total++;
            }*/
            packet_process(packet, 20+RAW_VIDEO_BLOCK_LENGTH);
            packet_process(packet, 20+RAW_VIDEO_BLOCK_LENGTH);
            input += RAW_VIDEO_BLOCK_LENGTH;
            length -= RAW_VIDEO_BLOCK_LENGTH;
            i++;
        } else {
            if (length==0) {
                break;
            }
            memcpy(packet+20, input, length);
            /*if (i < MAX_FRAME_PKT_BUF) {
                memcpy(&frame_pkt_buf[i][0], packet, 20+length);
                frame_pkt_buf_len[i]= 20+length;
                frame_pkt_buf_len_total++;
            }*/
            packet_process(packet, 20+length);
            packet_process(packet, 20+length);
            break;
        }
    }
    cur_pid+=block_num;
}
#endif
extern char router_list[2048];
int set_cds_spi(unsigned int l_cds){
    char *l_cmd=router_list;
    l_cmd[0] = CMD_CDS;
	l_cmd[1] = SET_ID;
	l_cmd[2] = (l_cds>>8)&0xFF;
	l_cmd[3] = l_cds&0xFF;
	spi_add_checksum(l_cmd);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	sendbuf_put_spi_buff(l_cmd);
	sendbuf_put_spi_buff(l_cmd);
	UART_PRINT("Set CDS triggered %dmV\r\n",l_cds);
}
int play_dingdong(void){
    char *l_cmd=router_list;
    l_cmd[0] = CMD_DINGDONG;
    l_cmd[1] = 0x00;
	spi_add_checksum(l_cmd);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	sendbuf_put_spi_buff(l_cmd);
//	sendbuf_put_spi_buff(l_cmd);
	UART_PRINT("\r\nPlay dingdong\r\n");
}
int voiceprompt_play(int l_option)
{
	int ret_val = 0;
	char *l_spi_buf=router_list;
	memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
	l_spi_buf[0] = CMD_VOICEPROMPT;
	l_spi_buf[1] = l_option;
	spi_add_checksum(l_spi_buf);
//	RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
	sendbuf_put_spi_buff(l_spi_buf);
	UART_PRINT("\r\nPlay %02x\r\n", l_option);
}
int set_volume(void){
	char *l_cmd=router_list;
	memset(l_cmd, 0x00, 1024);
	l_cmd[0] = CMD_VOLUME;
	l_cmd[1] = SET_ID;
	l_cmd[2] = cam_settings[9]; //Spk vol
	l_cmd[3] = cam_settings[10]; //Mic vol
	l_cmd[4] = 0; //Echo
	l_cmd[5] = 0; //Noise reduction
	spi_add_checksum(l_cmd);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	sendbuf_put_spi_buff(l_cmd);
	UART_PRINT("\r\nSet audio info %d %d %d %d\r\n", l_cmd[2], l_cmd[3], l_cmd[4], l_cmd[5]);
}
int set_uploadinfo(char* l_data, int len){
	char *l_cmd=router_list;
	l_cmd[0] = CMD_DINGDONG;
	l_cmd[1] = 0xA0;
	l_cmd[2] = len>>8;
	l_cmd[3] = len&0xff;
	memcpy(l_cmd+4,l_data,len);
	spi_add_checksum(l_cmd);
	sendbuf_put_spi_buff(l_cmd);
	UART_PRINT("\r\nUploadinfo: %s\r\n", l_cmd+4);
}
extern char sd_filename[];
extern int sd_file_status;
int spi_set_motion_file(){
	char *l_cmd=router_list;
	int l_temp;
    
	switch (sd_file_status) {
	    case 1: // Send filename to AK
	     	memset(l_cmd,0x00,1024);
	    	l_temp=strlen(sd_filename);
	    	l_cmd[0] = CMD_FILEUPLOAD;
	    	l_cmd[1]=0x01;
	    	l_cmd[2]=(l_temp>>8)&0xff;
	    	l_cmd[3]=(l_temp)&0xff;
	    	memcpy(l_cmd+4,sd_filename,l_temp);
	    	UART_PRINT("\r\nFilename to AK:%s\r\n",sd_filename);
	    	sd_file_status = 2;
	    	break;
	    case 5: // Send start to AK
	    	memset(l_cmd,0x00,1024);
	    	l_cmd[0] = CMD_FILEUPLOAD;
	    	l_cmd[1]=0x03;
	    	UART_PRINT("AK to start file\r\n");
	    	sd_file_status = 6;
	    	break;
	    case 7: // Send stop to AK
	    	memset(l_cmd,0x00,1024);
	    	l_cmd[0] = CMD_FILEUPLOAD;
	    	l_cmd[1]=0x05;
	    	UART_PRINT("\r\nStop filetransfer to AK\r\n");
	    	sd_file_status = 0;
	    	sd_filename[0]=0x00;
	    	break;
	    default:
	    	return 0;
	    	break;
		}
	spi_add_checksum(l_cmd);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	//RingBuffer_Write(ring_buffer_send, (char *)l_cmd, 1024);
	sendbuf_put_spi_buff(l_cmd);
	sendbuf_put_spi_buff(l_cmd);
	return 0;
}

int spi_get_motion_file(char* spi_response){
	int i;
	for(i=0;i<2;i++)
		UART_PRINT("\r\nAK return %d %d %d\r\n",sd_file_status, spi_response[1], spi_response[2]);
	if ((sd_file_status==2)&&(spi_response[1]==0x02)){
		if (spi_response[2]==0x01){//AK say filename valid
			sd_file_status=4;
		}else{ //AK say filename not valid
			sd_file_status=0;
			sd_filename[0]=0x00;
		}
		return 0;
	}
	if (spi_response[1]==0x04){
		sd_file_status=0;
		sd_filename[0]=0x00;
	}
}