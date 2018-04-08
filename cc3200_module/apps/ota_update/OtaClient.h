#ifndef __OTACLIENT_H__
#define __OTACLIENT_H__

#include "OtaClient.h"
#include "OtaUtil.h"
#include "Ota.h"
#include "CdnClient.h"

int8_t OtaVersion(otaInfo_t *pOtaInfo, cdnInfo_t *pCdnInfo, int l_cmd, int l_ota_status);

#endif