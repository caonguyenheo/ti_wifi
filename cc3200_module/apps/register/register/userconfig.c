#include <stdio.h>
#include <stdlib.h>
#include "userconfig.h"
#include "sflash.h"
/*Little edian */
UserConfig USERCONFIG; //the struct contain all user information
#if (PLATFORM == CC3200)
static long lFileHandle;
static unsigned long ulToken;
static int first = 1;
#endif
unsigned char *get_userconfig_pointer()
{
    return (unsigned char *)&USERCONFIG;
}
int get_userconfig_size()
{
    return sizeof(USERCONFIG);
}
static void convert_userconfig_uint32(int *result, int *len)
{
    *len = -1;
    if (!result)
        return;
    memcpy(result, &USERCONFIG, sizeof(USERCONFIG));
    *len = sizeof(USERCONFIG)/WORD_LEN;
}
static void convert_uint32_userconfig(int *result)
{
    if (!result)
        return;
    memcpy(&USERCONFIG, result, sizeof(USERCONFIG));

}

static int simple_cksum(unsigned char* arg, int len_of_arg)
{
    int cksum=0;
    int i;
    if(!arg)
        return -1;
    for (i=0;i<len_of_arg;i++)
    {
        cksum += arg[i];
    }
    return cksum;

}

/*
* Initation all infomation for userconfig
*
*/
int userconfig_init()
{
    memset(&USERCONFIG, 0, sizeof(USERCONFIG));
#if(PLATFORM == CC3200)
    InitFile(&ulToken, &lFileHandle);
#endif
   return 0;
}

static void feed_data(char *destination, char *source, int len)
{
    if(!destination || len<=0)
        return;
    if(source)
        memcpy(destination, source, len);
    else
        memset(destination, 0, len);
}
static void feed_len(int *destination, int len)
{
    if(!destination || len<=0)
        return;
    *destination = len;
}
static void feed_word(int *destination, int len)
{
    if(!destination)
        return;
    *destination = len;
}

static int need_checksum(int id)
{
    switch(id)
    {
        case MASTERKEY_ID:
        case TOKEN_ID:
            return 1;
        default:
            break;
    }
    return 0;
}
static int assign_information(int id, int *expected_len, int **len_desbuffer, char **data_desbuffer, int **checksum)
{
    switch(id)
    {
    case MODE_ID:
        *len_desbuffer = (int *)&(USERCONFIG.mode);
    break;
    case MASTERKEY_ID:
        *expected_len = MASTERKEY_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.master_key_len);
        *data_desbuffer = (char *)USERCONFIG.master_key;
        *checksum = (int *)&(USERCONFIG.master_key_checksum);
    break;
    case TOKEN_ID:
        *expected_len = CHANNELTOKEN_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.channel_token_len);
        *data_desbuffer = (char *)USERCONFIG.channel_token;
        *checksum = (int *)&(USERCONFIG.channel_token_checksum);
    break;
    case TIMEZONE_ID:
        *expected_len = TIMEZONE_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.time_zone_len);
        *data_desbuffer = (char *)USERCONFIG.time_zone;
    break;
#ifdef GSM_APN_SUPPORT
    case GSM_APN_ID:
        *expected_len = GSMAPN_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.gsm_apn_len);
        *data_desbuffer = (char *)USERCONFIG.gsm_apn;
    break;
#endif
#ifdef WIFI_CONFIG_SUPPORT
    case WIFI_SSID_ID:
        *expected_len = WIFI_SSID_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.wifi_ssid_len);
        *data_desbuffer = (char *)USERCONFIG.wifi_ssid;
    break;
    case WIFI_KEY_ID:
        *expected_len = WIFI_KEY_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.wifi_key_len);
        *data_desbuffer = (char *)USERCONFIG.wifi_key;
    break;
#endif
    case SERIAL_ID:
        *expected_len = SERIAL_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.serial_len);
        *data_desbuffer = (char *)USERCONFIG.serial;
    break;
    case DATECODE_ID:
        *expected_len = DATECODE_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.datecode_len);
        *data_desbuffer = (char *)USERCONFIG.datecode;
    break;
    // P2P_SERVER_IP_ID,
    // P2P_SERVER_PORT_ID,
    // P2P_KEY_ID,
    // P2P_RAN_NUM_ID,
    case P2P_SERVER_IP_ID:
    {
        *expected_len = P2P_SERVER_IP_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.p2p_server_ip_len);
        *data_desbuffer = (char *)USERCONFIG.p2p_server_ip;
    }
    break;
    case P2P_SERVER_PORT_ID:
    {
        *expected_len = P2P_SERVER_PORT_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.p2p_server_port_len);
        *data_desbuffer = (char *)USERCONFIG.p2p_server_port;
    }
    break;
    case P2P_KEY_ID:
    {
        *expected_len = P2P_KEY_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.p2p_key_len);
        *data_desbuffer = (char *)USERCONFIG.p2p_key;
    }
    break;
    case P2P_RAN_NUM_ID:
    {
        *expected_len = P2P_RAN_NUM_LEN;
        *len_desbuffer = (int *)&(USERCONFIG.p2p_ran_num_len);
        *data_desbuffer = (char *)USERCONFIG.p2p_ran_num;
    }
    break;
    case API_URL:
    {
       *expected_len = API_LEN;
       *len_desbuffer = (int *)&(USERCONFIG.api_len);
       *data_desbuffer = (char *)USERCONFIG.api_num;
    }
    break;
    case MQTT_URL:
    {
		*expected_len = MQTT_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_num;
    }
    break;
    case NTP_URL:
    {
    	*expected_len = NTP_LEN;
    	*len_desbuffer = (int *) &(USERCONFIG.ntp_len);
    	*data_desbuffer = (char *) USERCONFIG.ntp_num;
    }
    break;
    case RMS_URL:
    {
		*expected_len = RMS_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.rms_len);
		*data_desbuffer = (char *) USERCONFIG.rms_num;
    }
    break;
	case STUN_URL:
	{
		*expected_len = STUN_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.stun_len);
		*data_desbuffer = (char *) USERCONFIG.stun_num;
	}
	break;
	case CAM_ID:
	{
		*expected_len = CAM_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.cam_len);
		*data_desbuffer = (char *) USERCONFIG.cam_num;
	}
	break;
	case MQTT_CLIENTID:
	{
		*expected_len = MQTT_CLIENTID_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_clientid_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_clientid;
	}
	break;
	case MQTT_USER:
	{
		*expected_len = MQTT_USER_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_user_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_user;
	}
	break;
	case MQTT_PASS:
	{
		*expected_len = MQTT_PASS_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_pass_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_pass;
	}
	break;
	case MQTT_TOPIC:
	{
		*expected_len = MQTT_TOPIC_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_topic_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_topic;
	}
	break;
	case WIFI_SEC_ID:
	{
		*expected_len = WIFI_SEC_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.wifi_sec_len);
		*data_desbuffer = (char *) USERCONFIG.wifi_sec;
	}
	break;
	case MQTT_SERVERTOPIC_ID:
	{
		*expected_len = MQTT_SERVERTOPIC_LEN;
		*len_desbuffer = (int *) &(USERCONFIG.mqtt_servertopic_len);
		*data_desbuffer = (char *) USERCONFIG.mqtt_servertopic;
	}
	break;
    default:
        return -1;
    break;
    }
    return 0;
}

/*
* Set the value to each element of USERCONFIG. The value will be stored in RAM
* data_buffer: the value need to be set, if data_buffer == NULL && id==MODE_ID, this is the special case need to implement differ with other case
* data_len: the length of data need to be set
* id: define which element need to be set in USERCONFIG
*/
int userconfig_set(char *data_buffer, int data_len, int id)
{
    int expected_len;
    char *data_desbuffer = NULL;
    int *len_desbuffer = NULL;
    int *len_sourcebuffer = NULL;
    int *checksum = NULL;
    if(id>=MAX_ELEMENT || id<0)
        return -1;
    if(assign_information(id, &expected_len, &len_desbuffer, &data_desbuffer, &checksum)!=0)
        return -1;
    if(id == MODE_ID)//special case for mode field
    {
        if(len_desbuffer!=NULL)
        {
            *len_desbuffer = data_len;
            return 0;
        }else
        {
            return -1;
        }
    }
    if(!data_buffer)
        return -1;
    
    if(data_len > expected_len)
        return -1;
    /*Write len of buffer to userconfig*/
    if(len_desbuffer!=NULL)
        feed_len(len_desbuffer, data_len);

    /*Write real data buffer to userconfig*/
    if(data_desbuffer!=NULL)
        feed_data((char *)data_desbuffer, data_buffer, data_len);
    
#if (DEBUG_USER_CONFIG)
    {
        int i;
        // DUMP ARRAY
        UART_PRINT("\r\nDUMP ARRAY\r\n");
        UART_PRINT("LEN=%d\r\n", *len_desbuffer);
        for (i = 0; i < *len_desbuffer; i++) {
            UART_PRINT("%02X ", data_desbuffer[i]);
        }
        UART_PRINT("\r\nDUMP ARRAY\r\n");
    }
#endif

    /*Write checksum if needed*/
    if((need_checksum(id)) && (checksum!=NULL))
    {
        *checksum = simple_cksum(data_buffer, WORD_LEN);
    }
    return 0;
}
/*
* Get the value to each element of USERCONFIG.
* data_buffer: return buffer will be stored the value
* *data_len: return the length of data. if data_buffer == NULL && id==MODE_ID, 
*               this is the special case need to implement differ with other case
* id: define which element need to be get in USERCONFIG
*/
int userconfig_get(char *data_buffer, int buf_sz, int id)
{
    int expected_len;
    char *data_desbuffer = NULL;
    int *len_desbuffer = NULL;
    int *len_sourcebuffer = NULL;
    int *checksum = NULL;
    if(id>=MAX_ELEMENT || id<0)
        return -1;
    if(assign_information(id, &expected_len, &len_desbuffer, &data_desbuffer, &checksum)!=0)
        return -1;
    if(id == MODE_ID)//special case for mode field
    {
        if(len_desbuffer!=NULL)
            return *len_desbuffer;
        else
            return -1;
    }
    /*Get length of element*/
    /*Get the real data of element*/
    if (*len_desbuffer > buf_sz) {
        UART_PRINT("len_desbuffer=%d, buf_sz=%d\r\n", len_desbuffer, buf_sz);
        return -3;
    }
    if(data_desbuffer && len_desbuffer)
        feed_data(data_buffer, (char *)data_desbuffer, *len_desbuffer);
    else
        return -1;
    /*Get checksum if needed*/
    if(need_checksum(id) && checksum)
    {
        int ck = simple_cksum(data_buffer, WORD_LEN);
        if(ck != *checksum)
        {
            return -2; //check sum fail
        }
    }
    return *len_desbuffer;
   
}
/*
* Read/Update all value from flash to USERCONFIG
* 
*/
int userconfig_flash_read()
{
#if(PLATFORM==LINUX || PLATFORM==NRF51)
    memcpy(&USERCONFIG, FLASH_BASEADDR, sizeof(USERCONFIG));
#elif(PLATFORM==REALTEK)
    flash_t flash;
    flash_stream_read(&flash, FLASH_BASEADDR, sizeof(USERCONFIG), (uint8_t *)&USERCONFIG);
#elif(PLATFORM==CC3200)
    return ReadFileFromDevice(ulToken, lFileHandle, (unsigned char*) &USERCONFIG, sizeof(USERCONFIG));
#endif
    
    return 0;
}
#if(PLATFORM==REALTEK)
int userconfig_flash_earse()
{
    flash_t flash;
    flash_erase_sector(&flash, FLASH_BASEADDR);
    return 0;
}
#endif
/*
* Write all value from USERCONFIG to flash
* 
*/
int userconfig_flash_write()
{
#if(PLATFORM==LINUX)
    memcpy(FLASH_BASEADDR, &USERCONFIG,  sizeof(USERCONFIG));
#elif(PLATFORM==NRF51)
    uint32_t flash_array[sizeof(USERCONFIG)/4+1];
    int len;
    convert_userconfig_uint32(flash_array, &len);
    flash_page_erase(FLASH_BASEADDR);// need to earse all block before writting
    flash_write(FLASH_BASEADDR, flash_array, len);
#elif(PLATFORM==REALTEK)
    flash_t flash;
    userconfig_flash_earse();
    flash_stream_write(&flash, FLASH_BASEADDR, sizeof(USERCONFIG), (uint8_t *)&USERCONFIG);
#elif(PLATFORM==CC3200)
    
    return WriteFileToDevice(&ulToken, &lFileHandle, (unsigned char*) &USERCONFIG, sizeof(USERCONFIG));
#endif
    
    return 0;
}



