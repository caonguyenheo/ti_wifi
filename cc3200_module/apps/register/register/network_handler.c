//*****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// Simplelink includes 
#include "simplelink.h"

// driverlib includes 
#include "hw_types.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#define USING_STATIC_IP           0
// free-rtos/TI-rtos include
#ifdef SL_PLATFORM_MULTI_THREADED
#include "osi.h"
#endif

//#include "network_handler.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "timer_if.h"
#include "common.h"
#include "hl_apconfig.h"
#include "cc3200_system.h"
#include "network_if.h"
#include "xmlcreate.h"
#include "system.h"

//*****************************************************************************
//                 DATA STRUCTURE & DEFINATION ZONE -- Start
//*****************************************************************************
#define NUM_OF_RETRY    3


// Network App specific status/error codes which are used only in this file
typedef enum{
     // Choosing this number to avoid overlap w/ host-driver's error codes
    DEVICE_NOT_IN_STATION_MODE = -0x7F0,
    DEVICE_NOT_IN_AP_MODE = DEVICE_NOT_IN_STATION_MODE - 1,
    DEVICE_NOT_IN_P2P_MODE = DEVICE_NOT_IN_AP_MODE - 1,

    STATUS_CODE_MAX = -0xBB8
}e_NetAppStatusCodes;

//*****************************************************************************
//                 DATA STRUCTURE -- End
//*****************************************************************************

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern unsigned long  g_ulStatus;   /* SimpleLink Status */
extern unsigned long  g_ulStaIp;    /* Station IP address */
extern unsigned long  g_ulGatewayIP; /* Network Gateway IP address */
extern unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; /* Connection SSID */
extern unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; /* Connection BSSID */
extern volatile unsigned short g_usConnectIndex; /* Connection time delay index */
extern const char     pcDigits[]; /* variable used by itoa function */
// FIXME: DkS
char router_list[2048] = {0};

static Sl_WlanNetworkEntry_t g_NetEntries[SCAN_TABLE_SIZE];
//volatile unsigned char g_ucProvisioningDone = 0;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************
#endif //USE_FREERTOS


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
int Network_IF_WifiSetMode(int iMode)
{
  int lRetVal = -1;
  char str[10] = "EU"; 
  lRetVal = sl_Start(NULL, NULL, NULL);
  // desire mode duplicate with previous mode
  if(lRetVal == iMode)
  {
    UART_PRINT("Current mode available\n\r");
    return 0;
  }

  sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE, 2, str); 
  /**< set device in AP mode >*/
  if(ROLE_AP == iMode)
  {
    UART_PRINT("Switching to AP mode on application request\n\r");
    // Switch to AP role and restart
    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);
    hl_ap_config();

    /**< end config ssid name > */
    // restart
    lRetVal = sl_Stop(0xFF);
    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);
    lRetVal = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lRetVal);
    
    // Check if the device is up in AP Mode
    if (ROLE_AP == lRetVal)
    {
      // If the device is in AP mode, we need to wait for this event
      // before doing anything
      while(!IS_IP_ACQUIRED(g_ulStatus))
      {
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#else
        osi_Sleep(1);
#endif
      }
    }
    else
    {
      // We don't want to proceed if the device is not coming up in AP-mode
      ASSERT_ON_ERROR(DEVICE_NOT_IN_AP_MODE);
    }
    UART_PRINT("Re-started SimpleLink Device: AP Mode\n\r");
  }
  
  /**< set device in STA mode >*/
  else if(ROLE_STA == iMode)
  {
    UART_PRINT("Switching to STA mode on application request\n\r");
    // Switch to AP role and restart
    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);
    // restart
    lRetVal = sl_Stop(0xFF);
    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);
    lRetVal = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lRetVal);
    // Check if the device is in station again
    if (ROLE_STA != lRetVal)
    {
      // We don't want to proceed if the device is not coming up in STA-mode
      ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
    }
    UART_PRINT("Re-started SimpleLink Device: STA Mode\n\r");
  }
  
  /**< set device in P2P mode >*/
  else if(ROLE_P2P == iMode)
  {
    UART_PRINT("Switching to P2P mode on application request\n\r");
    // Switch to AP role and restart
    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);
    // restart
    lRetVal = sl_Stop(0xFF);
    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);
    lRetVal = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lRetVal);
    // Check if the device is in station again
    if (ROLE_P2P != lRetVal)
    {
      // We don't want to proceed if the device is not coming up in P2P-mode
      ASSERT_ON_ERROR(DEVICE_NOT_IN_P2P_MODE);
    }
    UART_PRINT("Re-started SimpleLink Device: P2P Mode\n\r");
  }
  
  else
  {
    UART_PRINT("Invalid Wifi mode\n\r");
    return -1;
  }
  return 0;
}

//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************

// static int 
// Network_IF_DisconnectFromAP()
// {
//   int lRetVal = 0;
  
//   if (IS_CONNECTED(g_ulStatus))
//   {
//     lRetVal = sl_WlanDisconnect();
//     if(0 == lRetVal)
//     {
//       // Wait
//       while(IS_CONNECTED(g_ulStatus))
//       {
// #ifndef SL_PLATFORM_MULTI_THREADED
//         _SlNonOsMainLoopTask();
// #else
//         osi_Sleep(1);
// #endif
//       }
//       return lRetVal;
//     }
//     else
//     {
//       return lRetVal;
//     }
//   }
//   else
//   {
//     return lRetVal;
//   }
 
// }

//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
int Network_IF_Connect2AP(char* ssid, char* pass, int retry)
{
	SlSecParams_t secParams = {0};
	long lRetVal = -1;
	int i,j;
	//
	// Connect to the Access Point
	//
	while((i<2)&&(lRetVal<0)){
		secParams.Key = (signed char*)pass;
		secParams.KeyLen = strlen(pass);
		secParams.Type = sec_string_to_num();
		lRetVal = Network_IF_ConnectAP(ssid,secParams);
		UART_PRINT("Connection to AP:%s, pass:%s, sec:%d, ret %d\r\n", ssid, secParams.Key, secParams.Type, lRetVal);
		i++;
	}
	return lRetVal;
}

int wifi_strength=-100;
int get_wifi_strength(){
    int lRetVal;
    SlGetRxStatResponse_t rxStatResp;
    lRetVal = sl_WlanRxStatGet(&rxStatResp,0);
    wifi_strength = rxStatResp.AvarageDataCtrlRssi;
    UART_PRINT("Wifi strength %d\r\n", wifi_strength);
    return ((wifi_strength+255)*100/255);
}
//****************************************************************************
//
//!    \brief This function connect to AP and set static ip
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
int Network_IF_Connect2AP_static_ip(char* ssid, char* pass, int retry)
{
    long lRetVal = 0;
    if (ssid == NULL || pass == NULL) {
        g_input_state=LED_STATE_NONE;
        return -1;
    }
    
    sl_WlanProfileDel(0xFF);    
    ConfigureSimpleLinkToDefaultState();
    InitializeAppVariables();
    lRetVal = Network_IF_WifiSetMode(ROLE_STA);
    if (lRetVal < 0) {
        return lRetVal;
    }

    lRetVal = Network_IF_Connect2AP(ssid, pass, 0);
    if (lRetVal < 0) {
        return lRetVal;
    }
    // Set connect policy
    // Auto + SmartConfig + fast connect
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 1, 0, 0, 1), NULL, 0);
    if (lRetVal < 0) {
        return lRetVal;
    }
    /*
    * Save connection profile
    */
    lRetVal = sl_WlanProfileDel(0xFF);    
    if (lRetVal < 0) {
        return lRetVal;
    }

    // Set nearly max priority
    // Initialize AP security params
    SlSecParams_t SecurityParams = {0}; // AP Security Parameters
    SecurityParams.Key = (signed char*)pass;
    SecurityParams.KeyLen = strlen(pass);
    // FIXME: DkS
    // SecurityParams.Type = SECURITY_TYPE;
    //SecurityParams.Type = SL_SEC_TYPE_WPA_WPA2;
    SecurityParams.Type = sec_string_to_num();
    lRetVal = sl_WlanProfileAdd((signed char*)ssid,
                      strlen(ssid), 0, &SecurityParams, 0, 6, 0);
    if (lRetVal < 0) {
        return lRetVal;
    }

    #if (USING_STATIC_IP)
    /*
    * Set static ip
    */
    // Get current ip
    uint8_t len = sizeof(SlNetCfgIpV4Args_t);
    uint8_t dhcpIsOn = 0;
    SlNetCfgIpV4Args_t ipV4 = {0};
    sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO,&dhcpIsOn,&len,(_u8 *)&ipV4);
                                      
    UART_PRINT("DHCP is %s IP %d.%d.%d.%d MASK %d.%d.%d.%d GW %d.%d.%d.%d DNS %d.%d.%d.%d\n",        
            (dhcpIsOn > 0) ? "ON" : "OFF",                                                           
            SL_IPV4_BYTE(ipV4.ipV4,3),SL_IPV4_BYTE(ipV4.ipV4,2),SL_IPV4_BYTE(ipV4.ipV4,1),SL_IPV4_BYTE(ipV4.ipV4,0), 
            SL_IPV4_BYTE(ipV4.ipV4Mask,3),SL_IPV4_BYTE(ipV4.ipV4Mask,2),SL_IPV4_BYTE(ipV4.ipV4Mask,1),SL_IPV4_BYTE(ipV4.ipV4Mask,0),         
            SL_IPV4_BYTE(ipV4.ipV4Gateway,3),SL_IPV4_BYTE(ipV4.ipV4Gateway,2),SL_IPV4_BYTE(ipV4.ipV4Gateway,1),SL_IPV4_BYTE(ipV4.ipV4Gateway,0),                 
            SL_IPV4_BYTE(ipV4.ipV4DnsServer,3),SL_IPV4_BYTE(ipV4.ipV4DnsServer,2),SL_IPV4_BYTE(ipV4.ipV4DnsServer,1),SL_IPV4_BYTE(ipV4.ipV4DnsServer,0));

    // Change to STATIC
    sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE,IPCONFIG_MODE_ENABLE_IPV4,sizeof(SlNetCfgIpV4Args_t),(_u8 *)&ipV4);
    ASSERT_ON_ERROR(lRetVal);
  #endif
    return lRetVal; 
}

//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
// int Network_IF_Init()
// {
//   int lRetVal = -1;
//   // Reset CC3200 Network state machine
//   // InitializeAppVariables();
//   lRetVal = ConfigureSimpleLinkToDefaultState();
//   if(lRetVal < 0)
//   {
//     if(DEVICE_NOT_IN_STATION_MODE == lRetVal)
//     {
//       UART_PRINT("Failed to configure the device in its default state\n\r");
//     }
//     LOOP_FOREVER();
//   }
//   UART_PRINT("Device is configured in default state \n\r");
//   //
//   // Assumption is that the device is configured in station mode already
//   // and it is in its default state
//   //
//   lRetVal = sl_Start(0, 0, 0);
//   if (lRetVal < 0 || ROLE_STA != lRetVal)
//   {
//     UART_PRINT("Failed to start the device \n\r");
//     LOOP_FOREVER();
//   }
  
//   UART_PRINT("Started SimpleLink Device: STA Mode\n\r");
//   return 0;
// }

//*****************************************************************************
//
//! \brief This function Get the Scan router Result
//!
//! \param[out]      netEntries - Scan router list Result 
//!
//! \return         Success - Size of Scan Result Array
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
static int 
Network_IF_GetScanRTResult(Sl_WlanNetworkEntry_t* netEntries)
{
  unsigned char   policyOpt;
  unsigned long IntervalVal = 10;
  int lRetVal;

  policyOpt = SL_CONNECTION_POLICY(0, 0, 0, 0, 0);
  lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION , policyOpt, NULL, 0);
  if(lRetVal < 0)
  {
    UART_PRINT("set connect policy failed\n");
    return lRetVal;
  }
  // enable scan
  policyOpt = SL_SCAN_POLICY(1);
  
  // set scan policy - this starts the scan
  lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                             (unsigned char *)(IntervalVal), sizeof(IntervalVal));
  if(lRetVal < 0)
  {
    UART_PRINT("set scan policy failed\n");
    return lRetVal;
  }
  
// extern long ConfigureSimpleLinkToDefaultState();
// extern void InitializeAppVariables();
  
  // delay 1 second to verify scan is started
  osi_Sleep(1000);

  // lRetVal indicates the valid number of entries
  // The scan results are occupied in netEntries[]
  lRetVal = sl_WlanGetNetworkList(0, SCAN_TABLE_SIZE, netEntries);
  if(lRetVal < 0)
  {
    UART_PRINT("get ap list failed\n");
    return lRetVal;
  }
  // Disable scan
  policyOpt = SL_SCAN_POLICY(0);
  
  // set scan policy - this stops the scan
  sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                   (unsigned char *)(IntervalVal), sizeof(IntervalVal));
  if(lRetVal < 0)
  {
    UART_PRINT("disable scan policy failed\n");
    return lRetVal;
  }
  
  return lRetVal;
  
}

//*****************************************************************************
//
//! \brief This function Get the Scan router Result
//!
//! \param[out]      netEntries - Scan router list Result 
//!
//! \return         Success - Size of Scan Result Array
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
Network_IF_provisionAP()
{
  long lCountSSID;
  long ssid_cnt,i,j;
  unsigned char temp_var;
  char mac_address[32] = "\0";
  //  static char router_list_resp[1024] = {0};
  char router_list_data[2048] = {0};
  int l_scantotalnum=0;
  
  char router_info_fm[200] = "<w>\n" \
    "<s>\"%s\"</s>\n" \
      "<b>%s</b>\n" \
        "<a>%s</a>\n" \
          "<q>%d</q>\n" \
            "<si>%d</si>\n" \
              "<nl>%d</nl>\n" \
                "<ch>%d</ch>\n" \
                  "</w>\n";
  
  char xml_header[100] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
    "<wl v=\"2.0\">\n"\
      "<n>%d</n>\n";
//  while(!g_ucProvisioningDone)
  for(j=0;j<5;j++){//j loop for number of rescan
  {   
    #if 1 
    //Get Scan Result
    lCountSSID = Network_IF_GetScanRTResult(&g_NetEntries[0]);

    if(lCountSSID < 0)
    {
      return lCountSSID;
    }
    #else
    lCountSSID = 0;
    #endif
//    UART_PRINT("<<------ List of Access Points (%d AP) ------>>\n", lCountSSID);
//    UART_PRINT("number of AP: %d\n", lCountSSID);
//    for(ssid_cnt = 0; ssid_cnt < lCountSSID; ssid_cnt++)
//    {
//      UART_PRINT("- AP ssid: %s\n", g_NetEntries[ssid_cnt].ssid);
//    }
    // assing header file for response data

    #if 1
    for(ssid_cnt = 0; ssid_cnt < lCountSSID; ssid_cnt++)
    {
      for(i = 0; i < 6; i++)
      {
        temp_var = g_NetEntries[ssid_cnt].bssid[i];
        //            UART_PRINT("%x ", temp_var);
        if(temp_var<0x0f)
        {
          sprintf(mac_address + strlen(mac_address), "0%x", temp_var);
        }else
        {
          sprintf(mac_address + strlen(mac_address), "%x", temp_var);
        }
        if(i<5)
        {
          sprintf(mac_address + strlen(mac_address), ":");
        }
       }
     // UART_PRINT("\r\n");
     // UART_PRINT("- ssid    : %s\r\n", g_NetEntries[ssid_cnt].ssid);
     // UART_PRINT("- bssid   : %s\r\n", mac_address);
     // UART_PRINT("- sec_type: %d\r\n", g_NetEntries[ssid_cnt].sec_type);
     // UART_PRINT("- rssi    : %d\r\n", g_NetEntries[ssid_cnt].rssi);
     // UART_PRINT("---------------------------------------------\r\n");
      char raw_ssid[SSID_LEN_MAX];
      char encode_ssid[SSID_LEN_MAX];
      memset(raw_ssid, 0x00, sizeof(raw_ssid));
      memset(encode_ssid, 0x00, sizeof(encode_ssid));
      sprintf(raw_ssid, "%.*s", g_NetEntries[ssid_cnt].ssid_len, g_NetEntries[ssid_cnt].ssid);
      write_xml_string(raw_ssid, encode_ssid);
      if ((l_scantotalnum>=SCAN_TABLE_SIZE) || (strstr(router_list_data,encode_ssid)!=NULL)){
          memset(mac_address, 0, sizeof(mac_address));
          continue;
      }
      UART_PRINT("ssid %d,%d: %s\r\n", ssid_cnt, l_scantotalnum, encode_ssid);
      // assign format for rounter info
      sprintf(router_list_data + strlen(router_list_data), router_info_fm, 
              encode_ssid,
              mac_address,
              (g_NetEntries[ssid_cnt].sec_type == 0)? "Open":
                (g_NetEntries[ssid_cnt].sec_type == 1)? "WEP":
                  (g_NetEntries[ssid_cnt].sec_type == 2)? "WPA":
                    "WPA2",
                    ((g_NetEntries[ssid_cnt].rssi+255)*100/255),
                    ((g_NetEntries[ssid_cnt].rssi+255)*100/255),
                    0,
                    0
                      );
      
      memset(mac_address, 0, sizeof(mac_address));
      l_scantotalnum++;
    }
    #endif 
    
    }//end for j
    memset(router_list, 0, sizeof(router_list));
    sprintf(router_list, xml_header, l_scantotalnum);
    memcpy(router_list + strlen(router_list), router_list_data, strlen(router_list_data));
    memcpy(router_list + strlen(router_list), "</wl>", strlen("</wl>"));
  }
}
