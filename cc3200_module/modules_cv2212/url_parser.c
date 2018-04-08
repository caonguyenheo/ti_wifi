#include "url_parser.h"

typedef struct {
	char data[32];
	int id;
}stringcode_struct;

typedef enum {
	ePARAM_ID_CMD=0,
	ePARAM_ID_VALUE,
	ePARAM_ID_LEVEL,
	ePARAM_ID_TIMEZONE,
	ePARAM_ID_SETUP,
	ePARAM_ID_CNT
} ePARAM_ID;

stringcode_struct command_map[eCOMMAND_ID_CNT]= {
	{"get_version", eCOMMAND_ID_GET_VERSION},
	{"get_mac_address", eCOMMAND_ID_GET_MAC},
	{"get_udid", eCOMMAND_ID_GET_UDID},
	{"check_fw_upgrade", eCOMMAND_ID_CHECK_FIRMWARE_UPGRADE},
	{"request_fw_upgrade", eCOMMAND_ID_REQUEST_FIRMWARE_UPGRADE},
	{"restart_system", eCOMMAND_ID_RESTART_SYSTEM},
	{"get_rt_list", eCOMMAND_ID_GET_RT_LIST},
	{"set_server_auth", eCOMMAND_ID_SET_SERVER_AUTH},
	{"setup_wireless_save", eCOMMAND_ID_SETUP_WIRELESS_SAVE},
	{"get_wifi_connection_state", eCOMMAND_ID_GET_WIFI_CONNECTION_STATE}
};

stringcode_struct key_map[ePARAM_ID_CNT]= {
	{"command", ePARAM_ID_CMD},
	{"value", ePARAM_ID_VALUE},
	{"level", ePARAM_ID_LEVEL},
	{"timezone", ePARAM_ID_TIMEZONE},
	{"setup", ePARAM_ID_SETUP}
};

urlResponse_struct urlResponse=default_urlResponse_struct;

//
int ParseWifiConfigString1(unsigned char * strDataIn, nxcNetwork_st *wifiConfig);
nxcNetwork_st wifiConfig_nonos;
//

int command_parser(char* input){
	char *cur_pos=input, *mid_pos=NULL;
	char *next_pos, *temp;
	int i, param_id, command_id;

	
	////printf("---3.1\n");
    urlResponse_struct default_urlResponse_struct_temp = default_urlResponse_struct;
	urlResponse = default_urlResponse_struct_temp;
	if (input[0]==0)
		return -1;
	////printf("---3.2\n");
	while (1){
		mid_pos = NULL;
		next_pos = cur_pos+1;
		param_id = -1;
		////printf("---3.3 %d %d\n",(int)(cur_pos-input),(int)(next_pos-input));
		while (*next_pos!='&' && *next_pos!=0)
		{
			if (*next_pos=='=')
				mid_pos = next_pos;
			next_pos++;
		}
		if (!((next_pos>mid_pos) && (mid_pos>cur_pos)))
			break;
		for (i=0;i<ePARAM_ID_CNT;i++){
			if (strstr(cur_pos, key_map[i].data)!=NULL){
				param_id = i;
				break;
			}
		}
		if (i==ePARAM_ID_CNT){
			if (*next_pos==0)
				break;
			else{
				cur_pos=next_pos+1;
				continue;
			}
		}
		////printf("---3.4 %d %s", i, key_map[i].data);
		switch(param_id){
			case ePARAM_ID_CMD:
				for (i=0;i<eCOMMAND_ID_CNT;i++){
					////printf("%s %s\n",mid_pos+1,command_map[i].data);
					if (strstr(mid_pos+1, command_map[i].data)!=NULL){
						urlResponse.has_command_id_parsed=1;
						urlResponse.command_id_parsed = i;
						break;
					}
				}
				break;
			case ePARAM_ID_VALUE:
				i=0;
				for (mid_pos=mid_pos+1;mid_pos<next_pos;mid_pos++){
					urlResponse.value_parsed[i]=*mid_pos;
					i++;
					if (i==(65-1))
						break;
				}
				urlResponse.value_parsed[i]=0;
				urlResponse.has_value_parsed = 1;
				break;
			case ePARAM_ID_LEVEL:
				i=0;
				for (mid_pos=mid_pos+1;mid_pos<next_pos;mid_pos++){
					urlResponse.level_parsed[i]=*mid_pos;
					i++;
					if (i==(17-1))
						break;
				}
				urlResponse.level_parsed[i]=0;
				urlResponse.has_level_parsed = 1;
				break;
			case ePARAM_ID_TIMEZONE:
				i=0;
				for (mid_pos=mid_pos+1;mid_pos<next_pos;mid_pos++){
					urlResponse.timezone_parsed[i]=*mid_pos;
					i++;
					if (i==(17-1))
						break;
				}
				urlResponse.timezone_parsed[i]=0;
				urlResponse.has_timezone_parsed = 1;
				break;
			case ePARAM_ID_SETUP:
				i=0;
				for (mid_pos=mid_pos+1;mid_pos<next_pos;mid_pos++){
					urlResponse.setup_parsed[i]=*mid_pos;
					i++;
					if (i==(129-1))
						break;
				}
				urlResponse.setup_parsed[i]=0;
				urlResponse.has_setup_parsed = 1;
				if ((urlResponse.has_command_id_parsed) && (urlResponse.command_id_parsed==eCOMMAND_ID_SETUP_WIRELESS_SAVE)){
					ParseWifiConfigString1(urlResponse.setup_parsed, &wifiConfig_nonos);
					////printf("SSID %s, Pass %s\n", wifiConfig.ESSID, wifiConfig.Key);
				}
					
				break;
		}
		if (*next_pos==0)
			break;
		else
			cur_pos=next_pos+1;
	}

	return 0;
}

void command_parser_unittest_setupstring(urlResponse_struct * origin_urlResponse, char * teststring){
	//printf("---2.1\n");
	if (origin_urlResponse->has_command_id_parsed){
		strcat(teststring,key_map[ePARAM_ID_CMD].data);
		strcat(teststring,"=");
		strcat(teststring, command_map[origin_urlResponse->command_id_parsed].data);
	}
	//printf("---2.2\n");
	if (origin_urlResponse->has_value_parsed){
		strcat(teststring,"&");
		strcat(teststring,key_map[ePARAM_ID_VALUE].data);
		strcat(teststring,"=");
		strcat(teststring, origin_urlResponse->value_parsed);
	}
	//printf("---2.3\n");
	if (origin_urlResponse->has_level_parsed){
		strcat(teststring,"&");
		strcat(teststring,key_map[ePARAM_ID_LEVEL].data);
		strcat(teststring,"=");
		strcat(teststring, origin_urlResponse->level_parsed);
	}
	//printf("---2.4\n");
	if (origin_urlResponse->has_timezone_parsed){
		strcat(teststring,"&");
		strcat(teststring,key_map[ePARAM_ID_TIMEZONE].data);
		strcat(teststring,"=");
		strcat(teststring, origin_urlResponse->timezone_parsed);
	}
	//printf("---2.5\n");
	if (origin_urlResponse->has_setup_parsed){
		strcat(teststring,"&");
		strcat(teststring,key_map[ePARAM_ID_SETUP].data);
		strcat(teststring,"=");
		strcat(teststring, origin_urlResponse->setup_parsed);
	}
	//printf("---2.6\n");
}
void command_parser_unittest_printbuf(urlResponse_struct * origin_urlResponse){
	//printf("Command found %d, val %s\n",origin_urlResponse->has_command_id_parsed, command_map[origin_urlResponse->command_id_parsed].data);	
	//printf("value found %d, val %s\n",origin_urlResponse->has_value_parsed, origin_urlResponse->value_parsed);
	//printf("level found %d, val %s\n",origin_urlResponse->has_level_parsed, origin_urlResponse->level_parsed);
	//printf("timezone found %d, val %s\n",origin_urlResponse->has_timezone_parsed, origin_urlResponse->timezone_parsed);
	//printf("setup found %d, val %s\n" ,origin_urlResponse->has_setup_parsed, origin_urlResponse->setup_parsed);
	//printf("SSID %s\n",wifiConfig_nonos.ESSID);
	//printf("PASSWORD %s\n",wifiConfig_nonos.Key);
}
int command_parser_unittest(){
	char teststring[512]={0};
	int i, ret;
	
	// Test 1
	//printf("---1\n");
	
        urlResponse_struct temp_struct=default_urlResponse_struct;
	urlResponse_struct origin_urlResponse=temp_struct;
	origin_urlResponse.has_command_id_parsed=1;
	origin_urlResponse.command_id_parsed=eCOMMAND_ID_SETUP_WIRELESS_SAVE;
	origin_urlResponse.has_value_parsed=1;
	strcpy(origin_urlResponse.value_parsed, "0123456789012345012345678901234501234567890123450123456789012345"); //64 chars
	origin_urlResponse.has_level_parsed=1;
	strcpy(origin_urlResponse.level_parsed, "01234567890123456");
	origin_urlResponse.has_timezone_parsed=1;
	strcpy(origin_urlResponse.timezone_parsed, "01234567890123456");
	origin_urlResponse.has_setup_parsed=1;
	//strcpy(origin_urlResponse.setup_parsed, "01234567890123450123456789012345012345678901234501234567890123450123456789012345012345678901234501234567890123450123456789012345"); //128 chars
	strcpy(origin_urlResponse.setup_parsed, "1002000120800000000606NXBroadbandV13579135camera000000");
	
	//printf("---2\n");
	
	command_parser_unittest_setupstring(&origin_urlResponse, teststring);
	//printf("---Source---\n");

	command_parser_unittest_printbuf(&origin_urlResponse);
	//printf("%s\n", teststring);
	//printf("---3\n");
	ret=command_parser(teststring);
	//printf("---Result--- %d\n",ret);
	command_parser_unittest_printbuf(&urlResponse);
	if (ret!=0)
		return -1;
	
	//printf("---4\n");
	
	if (origin_urlResponse.has_command_id_parsed!=urlResponse.has_command_id_parsed)
		return -1;
	if (origin_urlResponse.has_value_parsed!=urlResponse.has_value_parsed)
		return -1;
	if (origin_urlResponse.has_level_parsed!=urlResponse.has_level_parsed)
		return -1;
	if (origin_urlResponse.has_timezone_parsed!=urlResponse.has_timezone_parsed)
		return -1;
	if (origin_urlResponse.has_setup_parsed!=urlResponse.has_setup_parsed)
		return -1;
	// To do more compare
	//printf("---5\n");
	
	
	// Test 2
	//printf("---1\n");
	
	origin_urlResponse=temp_struct;
	origin_urlResponse.has_command_id_parsed=1;
	origin_urlResponse.command_id_parsed=eCOMMAND_ID_GET_VERSION;
	origin_urlResponse.has_value_parsed=0;
	strcpy(origin_urlResponse.value_parsed, "0123456789012345012345678901234501234567890123450123456789012345"); //64 chars
	origin_urlResponse.has_level_parsed=0;
	strcpy(origin_urlResponse.level_parsed, "01234567890123456");
	origin_urlResponse.has_timezone_parsed=0;
	strcpy(origin_urlResponse.timezone_parsed, "01234567890123456");
	origin_urlResponse.has_setup_parsed=0;
	strcpy(origin_urlResponse.setup_parsed, "01234567890123450123456789012345012345678901234501234567890123450123456789012345012345678901234501234567890123450123456789012345"); //128 chars

	
	//printf("---2\n");
	
	command_parser_unittest_setupstring(&origin_urlResponse, teststring);
	//printf("---Source---\n");

	command_parser_unittest_printbuf(&origin_urlResponse);
	//printf("%s\n", teststring);
	//printf("---3\n");
	ret=command_parser(teststring);
	//printf("---Result--- %d\n",ret);
	command_parser_unittest_printbuf(&urlResponse);
	if (ret!=0)
		return -1;
	
	//printf("---4\n");
	
	if (origin_urlResponse.has_command_id_parsed!=urlResponse.has_command_id_parsed)
		return -1;
	if (origin_urlResponse.has_value_parsed!=urlResponse.has_value_parsed)
		return -1;
	if (origin_urlResponse.has_level_parsed!=urlResponse.has_level_parsed)
		return -1;
	if (origin_urlResponse.has_timezone_parsed!=urlResponse.has_timezone_parsed)
		return -1;
	if (origin_urlResponse.has_setup_parsed!=urlResponse.has_setup_parsed)
		return -1;
	// To do more compare
	//printf("---5\n");
	
	
	// Test 3
	//printf("---1\n");
	
	origin_urlResponse=temp_struct;
	origin_urlResponse.has_command_id_parsed=1;
	origin_urlResponse.command_id_parsed=eCOMMAND_ID_GET_UDID;
	origin_urlResponse.has_value_parsed=0;
	strcpy(origin_urlResponse.value_parsed, "0123456789012345012345678901234501234567890123450123456789012345"); //64 chars
	origin_urlResponse.has_level_parsed=0;
	strcpy(origin_urlResponse.level_parsed, "01234567890123456");
	origin_urlResponse.has_timezone_parsed=0;
	strcpy(origin_urlResponse.timezone_parsed, "01234567890123456");
	origin_urlResponse.has_setup_parsed=0;
	strcpy(origin_urlResponse.setup_parsed, "01234567890123450123456789012345012345678901234501234567890123450123456789012345012345678901234501234567890123450123456789012345"); //128 chars

	
	//printf("---2\n");
	
	command_parser_unittest_setupstring(&origin_urlResponse, teststring);
	//printf("---Source---\n");

	command_parser_unittest_printbuf(&origin_urlResponse);
	//printf("%s\n", teststring);
	//printf("---3\n");
	ret=command_parser(teststring);
	//printf("---Result--- %d\n",ret);
	command_parser_unittest_printbuf(&urlResponse);
	if (ret!=0)
		return -1;
	
	//printf("---4\n");
	
	if (origin_urlResponse.has_command_id_parsed!=urlResponse.has_command_id_parsed)
		return -1;
	if (origin_urlResponse.has_value_parsed!=urlResponse.has_value_parsed)
		return -1;
	if (origin_urlResponse.has_level_parsed!=urlResponse.has_level_parsed)
		return -1;
	if (origin_urlResponse.has_timezone_parsed!=urlResponse.has_timezone_parsed)
		return -1;
	if (origin_urlResponse.has_setup_parsed!=urlResponse.has_setup_parsed)
		return -1;
	// To do more compare
	//printf("---5\n");
	
	// Test 4
	//printf("---1\n");
	
	origin_urlResponse=temp_struct;
	origin_urlResponse.has_command_id_parsed=1;
	origin_urlResponse.command_id_parsed=eCOMMAND_ID_SET_SERVER_AUTH;
	origin_urlResponse.has_value_parsed=1;
	strcpy(origin_urlResponse.value_parsed, "nsqeM4xh_rC4KM1WYg82"); //64 chars
	origin_urlResponse.has_level_parsed=0;
	strcpy(origin_urlResponse.level_parsed, "01234567890123456");
	origin_urlResponse.has_timezone_parsed=1;
	strcpy(origin_urlResponse.timezone_parsed, "+07.00");
	origin_urlResponse.has_setup_parsed=0;
	strcpy(origin_urlResponse.setup_parsed, "01234567890123450123456789012345012345678901234501234567890123450123456789012345012345678901234501234567890123450123456789012345"); //128 chars

	
	//printf("---2\n");
	
	command_parser_unittest_setupstring(&origin_urlResponse, teststring);
	//printf("---Source---\n");

	command_parser_unittest_printbuf(&origin_urlResponse);
	//printf("%s\n", teststring);
	//printf("---3\n");
	ret=command_parser(teststring);
	//printf("---Result--- %d\n",ret);
	command_parser_unittest_printbuf(&urlResponse);
	if (ret!=0)
		return -1;
	
	//printf("---4\n");
	
	if (origin_urlResponse.has_command_id_parsed!=urlResponse.has_command_id_parsed)
		return -1;
	if (origin_urlResponse.has_value_parsed!=urlResponse.has_value_parsed)
		return -1;
	if (origin_urlResponse.has_level_parsed!=urlResponse.has_level_parsed)
		return -1;
	if (origin_urlResponse.has_timezone_parsed!=urlResponse.has_timezone_parsed)
		return -1;
	if (origin_urlResponse.has_setup_parsed!=urlResponse.has_setup_parsed)
		return -1;
	// To do more compare
	//printf("---5\n");
	
	return 0;
}


int ParseWifiConfigString1(unsigned char * strDataIn, nxcNetwork_st *wifiConfig)
{
        int retVal = 0;
        unsigned char lenSSID,lenKey,lenIP,lenSubnet,lenGW,lenwp,lenUsrN,lenPassW;
        int data_len = strlen(strDataIn);
        int index,check_count = 0;
        char strInt[10];
        
        unsigned char * strData = strDataIn;
        if(wifiConfig == NULL)
                return -1;
        memset((char*)wifiConfig,0,sizeof(nxcNetwork_st));

        index = 0;
        //! get wifi mode
        switch (strData[index])
        {
                case '0':
                        wifiConfig->NetworkType = 0;
                        break;
                case '1':
                        wifiConfig->NetworkType = 1;
                        break;
                default:
                        wifiConfig->NetworkType = 2;
                        break;
        }
        index++;
        if(index > data_len)
        {
            return -1;
        }
        //! get channel to set in adhoc mode
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        wifiConfig->Channel = atoi(strInt);
        if((wifiConfig->Channel <= 0)||(14 < wifiConfig->Channel))
        {
                wifiConfig->Channel = WIFI_DEFAULT_ADHOC_CHANNEL;//default channel
        }
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get authentication mode
        switch (strData[index++])
        {
                case '0':
//                        wifiConfig->AuthenType = NETWORK_AUTH_NONE;
//                       break;
                case '1':
                        wifiConfig->AuthenType = 1;
                        break;
                case '2':
                        wifiConfig->AuthenType = 2;
                        break;
                
                default:
                        wifiConfig->AuthenType = 3;
                        break;
        }
        if(index > data_len)
        {
            return -1;
        }
        //get keyindex if in open or shared mode
        if ((wifiConfig->AuthenType == 1))
        {
                switch (strData[index]){
                        case '0':
                                wifiConfig->KeyIndex = 0;
                                break;
                        case '1':
                                wifiConfig->KeyIndex = 1;
                                break;
                        case '2':
                                wifiConfig->KeyIndex = 2;
                                break;
                        case '3':
                                wifiConfig->KeyIndex = 3;
                                break;
                        default:
                                break;
                }
        }
        index++;
        if(index > data_len)
        {
            return -1;
        }
        //get ip mode
        switch (strData[index++])
        {
                case '0':
                        wifiConfig->IPMode = 0;
                        break;
                case '1':
                        wifiConfig->IPMode = 1;
                        break;
                default:
                        break;
        }
        if(index > data_len)
        {
            return -1;
        }
        //get length ssid
        strncpy(strInt,strData+index,3);
        strInt[3] = '\0';
        lenSSID = atoi(strInt);
        index += 3;
        if(index > data_len)
        {
            return -1;
        }
        //get length key
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenKey = atoi(strInt);
        if(lenKey == 0)
                wifiConfig->AuthenType = 0;
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get length ip
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenIP = atoi(strInt);
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get length subnet mask
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenSubnet = atoi(strInt);
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get length default gateway
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenGW = atoi(strInt);
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get length working port
        strncpy(strInt,strData+index,1);
        strInt[1] = '\0';
        lenwp = atoi(strInt);
        index += 1;
        if(index > data_len)
        {
            return -1;
        }
        //get length username
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenUsrN = atoi(strInt);
        index += 2;
        if(index > data_len)
        {
            return -1;
        }
        //get length password
        strncpy(strInt,strData+index,2);
        strInt[2] = '\0';
        lenPassW = atoi(strInt);
        index += 2;
        if(index > data_len)
        {
                return -1;
        }
        //! Check data available
        check_count = index + lenSSID + lenKey + lenIP + lenSubnet + lenGW + lenwp + lenUsrN + lenPassW;
        ////printf("Dump len: lenSSID=%d + lenKey=%d + lenIP=%d + lenSubnet=%d + lenGW=%d + lenwp=%d+ lenUsrN=%d + lenPassW=%d\n",
        //    lenSSID,lenKey,lenIP,lenSubnet,lenGW,lenwp,lenUsrN,lenPassW);
        if(check_count != data_len)
        {
                //printf("Error->data_len=%d,correct_len=%d\n",data_len,check_count);
                return -1;
        }
        //get string ssid
        strncpy(wifiConfig->ESSID, strData+index, lenSSID);
        wifiConfig->ESSID[lenSSID] = '\0';
        index +=lenSSID;
        //get string key
        strncpy(wifiConfig->Key, strData+index, lenKey);
        wifiConfig->Key[lenKey] = '\0';
        index +=lenKey;
        //! Ignore if DHCP
        if (wifiConfig->IPMode == 1){
                strncpy(wifiConfig->DefaultIP,        strData+index,lenIP);
                wifiConfig->DefaultIP[lenIP] = '\0';
                index += lenIP;
                strncpy(wifiConfig->SubnetMask,        strData+index,lenSubnet);
                wifiConfig->SubnetMask[lenSubnet] = '\0';
                index += lenSubnet;
                strncpy(wifiConfig->Gateway,strData+index,lenGW);
                wifiConfig->Gateway[lenGW] = '\0';
                index += lenGW;
                if(lenwp!=0){
                        //strncpy(wifiConfig->WorkPort, strData+index,lenwp);
                        //wifiConfig->WorkPort[lenwp] = '\0';
                }
                else{
                        //strcpy(wifiConfig->WorkPort,"80");
                }
                index += lenwp;
        }
        else{
                //strcpy(wifiConfig->WorkPort,"80");
                index += lenIP + lenSubnet + lenGW + lenwp;
        }

        if(wifiConfig->NetworkType==1){
                //strncpy(wifiConfig->Username, strData+index,lenUsrN);
                //wifiConfig->Username[lenUsrN] = '\0';
                index += lenUsrN;
                //strncpy(wifiConfig->Password, strData+index,lenPassW);
                //wifiConfig->Password[lenPassW] = '\0';
                index += lenPassW;
        }
        else{
                index += lenUsrN + lenPassW;
        }
        //! Dump

        return retVal;
}
