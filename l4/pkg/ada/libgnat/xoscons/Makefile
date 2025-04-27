PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

include $(PKGDIR)/contrib.inc

MODE = host
HOST_LINK = 1
LIBS = -lgnat

ifeq ($(HOST_GNATVERSION),)
  SYSTEMS=
endif

TARGET = xoscons
SRC_ADA = xoscons.adb xutil.adb

vpath %.adb $(CONTRIB_DIR)
vpath %.ads $(CONTRIB_DIR)

include $(L4DIR)/mk/prog.mk
