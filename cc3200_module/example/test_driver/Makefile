#
# Nguyen Quoc Dinh <dinh.nguyen@nxcomm.com>
# Sept 2015
#

PROJECTROOT = ../..

PROJECT = testing

target=NONOS

TEST_DIR += ./flash \
	./flash/test \
	./flash/test/test_runners

CFLAGS+=-DUNITY_INCLUDE_CONFIG_H

UNITY_ROOT=$(PROJECTROOT)/tools/unity

UNITY_DIR = \
	$(UNITY_ROOT)/src \
	$(UNITY_ROOT)/extras/fixture/src

SRC_DIR += $(UNITY_DIR) $(TEST_DIR)
IPATH += $(UNITY_DIR) $(TEST_DIR)

include $(PROJECTROOT)/Makefile.include
