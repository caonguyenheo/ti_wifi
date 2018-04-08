#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <rgb_led.h>

#include "jsmn.h"

int8_t
_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

int8_t
_json_get_value(const char *json, jsmntok_t *tok, const char *key, char *value, uint32_t valLen) {
  if (_jsoneq(json, tok, key) == 0 && tok[1].type == JSMN_PRIMITIVE) {
    int32_t len = tok[1].end-tok[1].start;

    // check len
    if (len >= valLen)
      return -1;

    strncpy(value, (char*)(json+ tok[1].start), len);
    (value)[len] = '\0';
    // UART_PRINT("%s: %s\n", key, *value);
    return 0;
  }
  return -1;
}

int8_t RgbParse(const char *json, uint32_t jsonLen, Rgb_t *rgb)
{
    int count = 0;
    int i;
    int r;
    jsmn_parser par;
    jsmntok_t tok[128]; /* We expect no more than 128 tokens */

    char red[5], green[5], blue[5];

    jsmn_init(&par);
    r = jsmn_parse(&par, json, jsonLen, tok, sizeof(tok)/sizeof(tok[0]));
    if (r < 0) {
        return -1;
    }
    // Assume the top-level element is an object 
    if (r < 1 || tok[0].type != JSMN_OBJECT) {
        return -1;
    }
    // Loop over all keys of the root object 
    for (i = 1; i < r; i++) {
        if (_json_get_value(json, &tok[i], "r", red, sizeof(red)) == 0)
            count += 1;
        else if (_json_get_value(json, &tok[i], "g", green, sizeof(green)) == 0)
            count += 1;
        else if (_json_get_value(json, &tok[i], "b", blue, sizeof(blue)) == 0)
            count += 1;
    }
    
    // clean up malloc when parsing failed
    if (count < 3) {
        return -1;
    }

    int32_t iRed, iBlue, iGreen;
    char *ptr;

    iRed = strtol(red, &ptr, 10);
    iBlue = strtol(blue, &ptr, 10);
    iGreen = strtol(green, &ptr, 10);

    if (iRed < 0 || iRed > 255
      || iBlue < 0 || iBlue > 255
      || iGreen < 0 || iGreen > 255
      ) {
        return -1;
    }

    rgb->red = iRed;
    rgb->blue = iBlue;
    rgb->green = iGreen;

    return 0;
}