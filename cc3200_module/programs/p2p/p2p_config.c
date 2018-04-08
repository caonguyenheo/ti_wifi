#include "p2p_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
// SimpleLink include
#include "simplelink.h"
#include "user_common.h"
#include "url_parser.h"
#include "httpclientapp.h"
#include "cc3200_system.h"
#include "userconfig.h"
#include "p2p_main.h"

#include "mpu_wrappers.h"
// define default value from factory

#include "common.h"
#include "hw_types.h"
#include <driverlib/prcm.h>

extern char ps_ip[];
extern uint32_t ps_port;
extern char p2p_key_char[];
extern char p2p_rn_char[];
extern char p2p_key_hex[];
extern char p2p_rn_hex[];
#define REQUEST_PORT_IP     "/stream/geturl.json?mac=%s"
extern char relay_url_ch[];
extern char ps_url[];
extern uint32_t ps_ssl_port;
extern uint32_t ps_port;
extern char str_ps_port[];
#define STREAM_SERVER "%s:%s"


/*
#if 1
#define HARDCODE_PS_IP        "54.159.2.195"
#define HARDCODE_PS_PORT      8000
// #define HARDCODE_PS_IP        "54.173.89.126"
// #define HARDCODE_PS_PORT      8000

#define HARDCODE_KEY_CHAR     "ZdqG1TekIYV6R3eF"
#define HARDCODE_KEY_HEX      "5A6471473154656B4959563652336546"

#define HARDCODE_RN_CHAR      "eqrJCoaAc6hJ"
#define HARDCODE_RN_HEX       "6571724A436F61416336684A"
#else
#define HARDCODE_PS_IP        "54.173.89.126"
#define HARDCODE_PS_PORT      8000

#define HARDCODE_KEY_CHAR     "1dmJ5bWoEL0jR3eF"
#define HARDCODE_KEY_HEX      "31646D4A3562576F454C306A52336546"

#define HARDCODE_RN_CHAR      "gqnMGGSE8TmW"
#define HARDCODE_RN_HEX       "67716E4D4747534538546D57"
#endif
*/

/*******************************************************************************
* @brief: Get MAC address and UID from device factory config
* @param: None
* @ret  : None
*/
void
config_init(void)
{
  static uint8_t is_init = 0;
  if (is_init == 1) {
    return;
  }
  // FIXME: DkS hardcode
  // init key
#if 0
    memcpy(ps_ip, HARDCODE_PS_IP, sizeof(HARDCODE_PS_IP));
    ps_port = HARDCODE_PS_PORT; 

    memcpy(p2p_key_char, HARDCODE_KEY_CHAR, sizeof(HARDCODE_KEY_CHAR));
    memcpy(p2p_key_hex, HARDCODE_KEY_HEX, sizeof(HARDCODE_KEY_HEX));

    memcpy(p2p_rn_char, HARDCODE_RN_CHAR, sizeof(HARDCODE_RN_CHAR));
    memcpy(p2p_rn_hex, HARDCODE_RN_HEX, sizeof(HARDCODE_RN_HEX));
#else
    int ret_val = 0;
    // RESET VARIABLE
    memset(ps_ip, 0x00, P2P_PS_IP_LEN);
    ps_port = 0;
    memset(p2p_key_char, 0x00, P2P_KEY_CHAR_LEN);
    memset(p2p_rn_char, 0x00, P2P_RN_CHAR_LEN);
    // GET CONFIG
    ret_val = userconfig_get(ps_ip, P2P_PS_IP_LEN, P2P_SERVER_IP_ID);
    if (ret_val <= 0) {
      UART_PRINT("Cannot read P2P ps_ip, error=%d\r\n", ret_val);
      return;
    }
    ret_val = userconfig_get(str_ps_port, P2P_PS_PORT_LEN, P2P_SERVER_PORT_ID);
    ps_port = atoi(str_ps_port);
    if (ret_val <= 0) {
      UART_PRINT("Cannot read P2P ps_port, error=%d\r\n", ret_val);
      return;
    }
    ret_val = userconfig_get(p2p_key_char, P2P_KEY_CHAR_LEN, P2P_KEY_ID);
    if (ret_val <= 0) {
      UART_PRINT("Cannot read P2P ps_key_char, error=%d\r\n", ret_val);
      return;
    }
    ret_val = userconfig_get(p2p_rn_char, P2P_RN_CHAR_LEN, P2P_RAN_NUM_ID);
    if (ret_val <= 0) {
      UART_PRINT("Cannot read P2P ps_rn_char, error=%d\r\n", ret_val);
      return;
    }

    char_to_hex(p2p_key_char, p2p_key_hex, strlen(p2p_key_char));
    char_to_hex(p2p_rn_char, p2p_rn_hex, strlen(p2p_rn_char));
#endif
    is_init = 1;
}

/*******************************************************************************
* @brief: Generate random alpha numberical case sensity char array (0-9, a-z, A-Z)
* @param[out]: output  - output buffef
* @param[in]:  len     - length of output
* @ret:        None 
*/
void
random_char(char *output,  int len)
{
  int i;
  char dev_mac[13] = {0};
  get_mac_address(dev_mac);
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (i = 0; i < len - 1; i++) {
    output[i] = rand();
    if ( 0 <= i && 12 > i ) {
      output[i] += dev_mac[i];
    }
    output[i] = charset[output[i] % strlen(charset)];
  }

}

/*******************************************************************************
* @brief: Convert char string to hex string 
* @param[in]:  input   - input buffer
* @param[out]: output  - output buffer
* @param[in]:  inp_len - length of input buffer
* @ret  :      0 if success, others for failure 
*/
int
char_to_hex(char *input, char *output, int inp_len)
{
  int i;
  char buf_ptr[3] = {0};

  if(NULL == input || 0 >= inp_len) {
    return -1;
  }
  for(i = 0; i < inp_len; i++) {
    snprintf(buf_ptr, sizeof buf_ptr, "%02X", input[i]);
    strncpy(output + i*2, buf_ptr, strlen(buf_ptr));
  }
  return 0;
}

/*******************************************************************************
* @brief: P2P init 
* @param: None
* @ret  : None
*/
#define REGISTER_DEVICE     "{\"mac\":\"%s\",\"rn\":\"%s\",\"key\":\"%s\",\"udid\":\"%s\"}"
#define POST_URI            "/stream/register.json"

void init_seed() {
  uint32_t cur_date_s = 0;
  uint16_t cur_date_ms = 0;
  uint32_t seed = 0;
  PRCMRTCGet(&cur_date_s, &cur_date_ms);
  seed += cur_date_ms;
  PRCMRTCGet(&cur_date_s, &cur_date_ms);
  seed += cur_date_ms;
  PRCMRTCGet(&cur_date_s, &cur_date_ms);
  seed += cur_date_ms;
  UART_PRINT("cur_date_ms=%d\r\n", seed);
    srand(seed);
}

int p2p_init(int update_param)
{
  /*char server_ip[30] = {0};*/
  char server_url[100] = {0};
  char server_port[10] = {0};
//  char server_stream_port[10] = {0};
  char uid[MAX_UID_SIZE] = {0};
  char device_mac[MAX_MAC_ADDR_SIZE] = {0};
  char resp_buf[2048] = {0}; 
  char request_buf[512] = {0};
  
  int ret_val = -1, ret_val_2 = -1;
  HTTPCli_Struct http_cli;
  
  if (update_param){
    init_seed();
    random_char(p2p_key_char, MAX_KEY_SIZE);
  
    random_char(p2p_rn_char, MAX_RN_SIZE);
    char_to_hex(p2p_key_char, p2p_key_hex, strlen(p2p_key_char));
    char_to_hex(p2p_rn_char, p2p_rn_hex, strlen(p2p_rn_char));
  } else
    config_init();
  UART_PRINT("\r\n random char string %s hex string %s\r\n", p2p_rn_char, p2p_rn_hex);
  UART_PRINT("\r\n key char string %s hex string: %s\r\n", p2p_key_char, p2p_key_hex); 
  get_mac_address(device_mac);
  get_uid(uid); 

  snprintf(request_buf, sizeof request_buf, REQUEST_PORT_IP, device_mac);
  UART_PRINT("\r\nrelay_url_ch: %s\r\n", relay_url_ch);
  ret_val = myCinHttpConnect(&http_cli, relay_url_ch, 443, 1);
  if (ret_val<0){
    myCinHttpClose(&http_cli);
    return ret_val;
  }
  
  UART_PRINT("\r\nrequest_buf: %s\r\n", request_buf);
  ret_val = OtaHttpSendRequest(&http_cli, relay_url_ch, request_buf, HTTPCli_METHOD_GET);
  ret_val_2 = myCinHttpVersionGetResponse(&http_cli, resp_buf, sizeof(resp_buf));
  myCinHttpClose(&http_cli);
  if ((ret_val < 0) && (ret_val_2<0)) {
    UART_PRINT("HTTP fails %d %d\r\n", ret_val, ret_val_2);
    return -1;
  }  
  UART_PRINT("request_buf %s Response: %s\r\n", request_buf, resp_buf);
  // parse get server ip
  {
  url_parser url_par;

  if (parse_url(resp_buf, &url_par)<0)
    return -1;
#ifdef P2P_FORCE_EU
  memset(ps_ip,0x00,P2P_PS_IP_LEN);
  memcpy(ps_ip,"35.159.10.137",strlen("35.159.10.137"));
  memset(ps_url,0x00,P2P_PS_IP_LEN);
  memcpy(ps_url,"rss4-lucy-reg02.lucyconnect.com",P2P_PS_URL_LEN);
#else
  if (getValueFromKey(&url_par, "sip", ps_ip, P2P_PS_IP_LEN)<0)
    return -1;
  if (getValueFromKey(&url_par, "surl", ps_url, P2P_PS_URL_LEN)<0)
    return -1;
#endif  
  UART_PRINT("\r\nsip %s surl %s\r\n", ps_ip, ps_url);
  
#ifdef P2P_FORCE_EU
  ps_ssl_port = 443;
  memset(str_ps_port,0x00,P2P_PS_PORT_LEN);
  memcpy(str_ps_port,"8000",strlen("8000"));
#else
  if (getValueFromKey(&url_par, "ssl_port", server_port, 10)<0)
    return -1;
  ps_ssl_port = atoi(server_port);
  if (getValueFromKey(&url_par, "udpstream_port", str_ps_port, P2P_PS_PORT_LEN)<0)
    return -1;
#endif
  ps_port = atoi(str_ps_port);
  UART_PRINT("PS ssl_port %d\r\n",ps_ssl_port);
  UART_PRINT("PS udp_port %d\r\n",ps_port);
  }
  
  memset(request_buf, 0, sizeof request_buf);
  snprintf(server_url, sizeof server_url, STREAM_SERVER, ps_url, server_port);
  snprintf(request_buf, sizeof request_buf, REGISTER_DEVICE,
                                            device_mac,
                                            p2p_rn_hex,
                                            p2p_key_hex,
                                            uid
    );
  memset(resp_buf, 0, sizeof resp_buf);
  UART_PRINT("server url: %s \r\n", server_url);
  UART_PRINT("server request: %s \r\n", request_buf);

  ret_val = myCinHttpConnect(&http_cli, ps_url, ps_ssl_port, 1);
  if (ret_val<0){
    myCinHttpClose(&http_cli);
    return ret_val;
  }
  
  //ret_val = HTTP_Post(&http_cli, ps_url, POST_URI, request_buf,resp_buf);
  ret_val = myCinHttpSendRequest(&http_cli, ps_url, POST_URI, HTTPCli_METHOD_POST, request_buf);
  ret_val_2 = myCinHttpVersionGetResponse(&http_cli, resp_buf, sizeof(resp_buf));
  myCinHttpClose(&http_cli);
  if ((ret_val < 0) || (ret_val_2<0)) {
    UART_PRINT("HTTP request fails %d %d\r\n", ret_val, ret_val_2);
    return -1;
  } 
  UART_PRINT("\r\nResponse: %s, code %d %d\r\n", resp_buf, ret_val, ret_val_2);
  if (strstr(resp_buf,"200")==NULL)
      return -1;
  UART_PRINT("\r\nP2P registration done\r\n");
  return 0;
}
