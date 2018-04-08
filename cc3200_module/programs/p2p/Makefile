#
# Nguyen Duy Khanh <khanh.nguyen@nxcomm.com>
# Jul 2016
#

PROJECTROOT = ../..

PROJECT = p2p

NETAPPS += paho_client
NETAPPS += ntp
NETAPPS += url_parser
NETAPPS += wima
NETAPPS += p2p_streaming
NETAPPS += httpclient_os
NETAPPS += inet
NETAPPS += aes
NETAPPS += cc3200
NETAPPS += sleep_profile
NETAPPS += serverapi
NETAPPS += notification
NETAPPS += ota_update
NETAPPS += mobile
# NETAPPS += spi_dma

USE_FREERTOS = 1
USE_CV2212_MODULE = 0

COMMON_SRC += network_if.c
COMMON_SRC += udma_if.c
COMMON_SRC += network_common.c
# LPDS_MODE = 0 -> hibernate mode
# LPDS_MODE = 1 -> Low Power Deep Sleep mode mode
CFLAGS += -DLPDS_MODE=1

include $(PROJECTROOT)/Makefile.include
