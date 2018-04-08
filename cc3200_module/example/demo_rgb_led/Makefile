#
# Nguyen Quoc Dinh <dinh.nguyen@nxcomm.com>
# Sept 2015
#

PROJECTROOT = ../..

PROJECT = mobile_registration

USE_FREERTOS = 1

# COMMON_SRC += network_if.c

# we need special mobile makefile
include ./Makefile.mobile 

NETAPPS += jsmn

# device registration, which include httpclient
NETAPPS += serverapi

# mqtt client for nxcomm, which include paho mqtt client
NETAPPS += paho_client

include $(PROJECTROOT)/Makefile.include
