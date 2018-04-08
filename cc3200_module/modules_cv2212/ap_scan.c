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
#include "ap_scan.h"

//*****************************************************************************
//                 DATA STRUCTURE & DEFINATION ZONE -- Start
//*****************************************************************************
#define MAX_ROUTER_LIST_BUF_SIZE   2048
#define RT_MAC_ADDR_SIZE           33

#define ROUTER_INFO_FM "<w>\n" \
    "<s>\"%.*s\"</s>\n" \
      "<b>%s</b>\n" \
        "<a>%s</a>\n" \
          "<q>%d</q>\n" \
            "<si>%d</si>\n" \
              "<nl>%d</nl>\n" \
                "<ch>%d</ch>\n" \
                  "</w>\n"
  
#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
    "<wl v=\"2.0\">\n"\
      "<n>%d</n>\n"


//*****************************************************************************
//
//! \brief This function Get the Scan router Result
//!
//! \param[out]      router_lst - router list result 
//! \param[int]      maxScanList - maximum of router in List can scan
//!
//! \return         Success - Size of Scan Result Array
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_get_list(Sl_WlanNetworkEntry_t* router_lst, 
                 int                    max_scan_list)
{
  unsigned char   policyOpt;
  unsigned long IntervalVal = 60;
  int lRetVal;
  
  policyOpt = SL_CONNECTION_POLICY(0, 0, 0, 0, 0);
  lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION , policyOpt, NULL, 0);
  if(lRetVal < 0){
    UART_PRINT("set connect policy failed\n");
    return lRetVal;
  }
  
  // enable scan
  policyOpt = SL_SCAN_POLICY(1);
  
  // set scan policy - this starts the scan
  lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                             (unsigned char *)(IntervalVal), sizeof(IntervalVal));
  if(lRetVal < 0){
    UART_PRINT("set scan policy failed\n");
    return lRetVal;
  }
  
  
  // delay 1 second to verify scan is started
  osi_Sleep(1000);
  
  // lRetVal indicates the valid number of entries
  // The scan results are occupied in netEntries[]
  lRetVal = sl_WlanGetNetworkList(0, max_scan_list, router_lst);
  if(lRetVal < 0){
    UART_PRINT("get ap list failed\n");
    return lRetVal;
  }
  
  // Disable scan
  policyOpt = SL_SCAN_POLICY(0);
  
  // set scan policy - this stops the scan
  sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                   (unsigned char *)(IntervalVal), sizeof(IntervalVal));
  if(lRetVal < 0){
    UART_PRINT("disable scan policy failed\n");
    return lRetVal;
  }
  
  return lRetVal;
  
}

//*****************************************************************************
//
//! \brief This function to convert router list array to xml format
//!
//! \param[in]      router_lst - router list result 
//! \param[in]      num_of_router - number of routers in list 
//! \param[out]     xml_ret_buf - string buffer return in xml format for router 
//!                 list 
//! \param[in]      xml_ret_buf_size - max size of return buffer 
//!
//! \return         Success - 0
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_list_to_xml(Sl_WlanNetworkEntry_t* router_lst, 
                    int                    num_of_router, 
                    char*                  xml_ret_buf, 
                    int                    xml_ret_buf_size)
{
  long ssid_cnt,i;
  unsigned char temp_var;
  char mac_address[RT_MAC_ADDR_SIZE] = "\0";
  char router_list_data[MAX_ROUTER_LIST_BUF_SIZE] = {0};
  
  // assing header file for response data
  memset(xml_ret_buf, 0, xml_ret_buf_size);
  memset(router_list_data, 0, sizeof(router_list_data));
  snprintf(xml_ret_buf, xml_ret_buf_size, XML_HEADER, num_of_router);
  
  for(ssid_cnt = 0; ssid_cnt < num_of_router; ssid_cnt++)
  {
    for(i = 0; i < 6; i++)
    {
      temp_var = router_lst[ssid_cnt].bssid[i];
      //            UART_PRINT("%x ", temp_var);
      if(temp_var<0x0f)
      {
        snprintf(mac_address + strlen(mac_address), RT_MAC_ADDR_SIZE, "0%x", temp_var);
      }else
      {
        snprintf(mac_address + strlen(mac_address), RT_MAC_ADDR_SIZE, "%x", temp_var);
      }
      if(i<5)
      {
        snprintf(mac_address + strlen(mac_address), RT_MAC_ADDR_SIZE, ":");
      }
     }
    
    // assign format for rounter info
    snprintf(router_list_data + strlen(router_list_data), MAX_ROUTER_LIST_BUF_SIZE, 
            ROUTER_INFO_FM, 
            router_lst[ssid_cnt].ssid_len, router_lst[ssid_cnt].ssid,
            mac_address,
            (router_lst[ssid_cnt].sec_type == 0)? "Open":
              (router_lst[ssid_cnt].sec_type == 1)? "WEP":
                (router_lst[ssid_cnt].sec_type == 2)? "WPA":
                  "WPA2",
                  -(router_lst[ssid_cnt].rssi),
                  router_lst[ssid_cnt].rssi,
                  0,
                  0
                    );
    
    memset(mac_address, 0, sizeof(mac_address));
  }
  memcpy(xml_ret_buf + strlen(xml_ret_buf), router_list_data,
         strlen(router_list_data));
  memcpy(xml_ret_buf + strlen(xml_ret_buf), "</wl>", strlen("</wl>"));
  return 0;
}

//*****************************************************************************
//
//! \brief This function to get router list info array in  xml format
//!
//! \param[out]     router_lst - router list result 
//! \param[in]      max_scan_lst - number of routers in list 
//! \param[out]     xml_ret_buf - string buffer return in xml format for router 
//!                 list 
//! \param[in]      xml_ret_buf_size - max size of return buffer 
//!
//! \return         Success - 0
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_mycin_format(int     max_scan_lst,
	                  char*   xml_ret_buf, 
                      int     xml_ret_buf_size)
{
	int num_of_router;
	int ret_val = -1;
	Sl_WlanNetworkEntry_t router_lst[max_scan_lst];

    num_of_router = ap_scan_get_list(router_lst, max_scan_lst);
    if(num_of_router <= 0){
    	return -1;
    }
    ret_val = ap_scan_list_to_xml(router_lst, num_of_router, xml_ret_buf, xml_ret_buf_size);		
    return ret_val;   
}
