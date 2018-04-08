#include <stdint.h>
#include <string.h>

#include "server_api.h"
#include "jsmn.h"
#include "userconfig.h"
//#include "common.h"
static int8_t
_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (((tok->type == JSMN_STRING) || (tok->type == JSMN_PRIMITIVE))&& (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

static int8_t
_json_get_value(const char *json, jsmntok_t *tok, const char *key, char *value, uint32_t valLen) {
  if (_jsoneq(json, tok, key) == 0 && (tok[1].type == JSMN_STRING || tok[1].type == JSMN_PRIMITIVE)) {
    int32_t len = tok[1].end-tok[1].start;

    // check len
    if (len >= valLen)
      return -1;

    strncpy(value, (char*)(json+ tok[1].start), len);
    //UART_PRINT("\r\nLongString %s\r\n", (char*)(json+ tok[1].start));

    (value)[len] = '\0';
     //UART_PRINT("\r\n%s: %s\r\n", key, *value);
    return 0;
  }
  return -1;
}

int8_t
myCinRegisterParse(const char *json, uint32_t json_len, myCinUserInformation_t *pmyCinUser,  myCinApiRegister_t *pmyCinRegister)
{
  int count = 0;
  int i;
  int r;
  jsmn_parser par;
  jsmntok_t tok[128]; /* We expect no more than 128 tokens */

  jsmn_init(&par);
  r = jsmn_parse(&par, json, json_len, tok, sizeof(tok)/sizeof(tok[0]));
  if (r < 0) {
    return -1;
  }
  // Assume the top-level element is an object 
  if (r < 1 || tok[0].type != JSMN_OBJECT) {
    return -1;
  }
  // Loop over all keys of the root object 
  for (i = 1; i < r; i++)
  {
	  if (_json_get_value(json, &tok[i], "device_token", pmyCinUser->authen_token, sizeof(pmyCinUser->authen_token)) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "client_id", pmyCinRegister->mqtt_clientid, MQTT_CLIENTID_LEN+1) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "access_key", pmyCinRegister->mqtt_user, MQTT_USER_LEN+1) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "secret_key", pmyCinRegister->mqtt_pass, MQTT_PASS_LEN+1) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "subscribe_topic", pmyCinRegister->mqtt_topic, MQTT_TOPIC_LEN+1) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "publish_cmd_topic", pmyCinRegister->mqtt_servertopic, MQTT_TOPIC_LEN+1) == 0)
	  {
		  count += 1;
	  }
	  /*else if (_json_get_value(json, &tok[i], "checksum", pmyCinUser->fw_checksum, sizeof(pmyCinUser->fw_checksum)) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "files", pmyCinUser->fw_files, sizeof(pmyCinUser->fw_files)) == 0)
	  {
		  count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "version", pmyCinUser->fw_version, sizeof(pmyCinUser->fw_version)) == 0)
	  {
	      count += 1;
	  }
	  else if (_json_get_value(json, &tok[i], "fw_id", pmyCinUser->fw_id, sizeof(pmyCinUser->fw_id)) == 0)
	  {
		  count += 1;
	  }*/
  }
  // clean up malloc when parsing failed
  	  if (count < 5)
  	  {
  		  return -1;
  	  }

  return 0;
}
extern char g_fw_id[];
int8_t
myCinOtaParse(const char *json, uint32_t json_len, ota_firmware_t *l_ota_firmware)
{
	int count = 0;
	int i;
	int r;
	jsmn_parser par;
	jsmntok_t tok[128]; // We expect no more than 128 tokens
	int current_fw_id=0;
	jsmn_init(&par);
	r = jsmn_parse(&par, json, json_len, tok, sizeof(tok)/sizeof(tok[0]));
	if (r < 0) {
		return -1;
	}
	// Assume the top-level element is an object 
	if (r < 1 || tok[0].type != JSMN_OBJECT) {
		return -1;
	}
	// Loop over all keys of the root object 
	for (i = 1; i < r; i++)
	{
		if (_json_get_value(json, &tok[i], "index", (l_ota_firmware->ota_firmware[current_fw_id]).fw_id, MAX_FW_ID) == 0)
		{
			if (current_fw_id==0)
				count = 1;
			else
				count += 1;
		}
		else if (_json_get_value(json, &tok[i], "version", (l_ota_firmware->ota_firmware[current_fw_id]).fw_version, MAX_FW_VERSION) == 0)
		{
			count += 1;
		}
		else if (_json_get_value(json, &tok[i], "pkg", (l_ota_firmware->ota_firmware[current_fw_id]).fw_files, MAX_FW_FILES) == 0)
		{
			count += 1;
		}
		else if (_json_get_value(json, &tok[i], "md5", (l_ota_firmware->ota_firmware[current_fw_id]).fw_checksum, MAX_FW_CHECKSUM) == 0)
		{
			count += 1;
			current_fw_id++;
		}
		else if (_json_get_value(json, &tok[i], "fw_id", g_fw_id, MAX_G_FW_ID) == 0)
		{
		}
	}
	return count;
}