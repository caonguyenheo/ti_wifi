#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// json parsing
#include "jsmn.h"

#include "OtaUtil.h"

#define FAILED    (-1)
#define SUCCESS   0

static int32_t
_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

static int32_t
_json_get_string_value(const char *json, jsmntok_t *tok, const char *key, char *value, uint32_t valLen) {
  if (_jsoneq(json, tok, key) == 0 && tok[1].type == JSMN_STRING) {
    uint32_t len = tok[1].end-tok[1].start;

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




