PKGDIR  ?= ..
L4DIR   ?= $(PKGDIR)/../../..

include $(PKGDIR)/contrib.inc

MODE = host
HOST_LINK = 1
LIBS = -lgnat

ifeq ($(HOST_GNATVERSION),)
  SYSTEMS=
endif

TARGET = xsnamest
SRC_ADA = xsnamest.adb xutil.adb

vpath %.adb $(CONTRIB_DIR)

include $(L4DIR)/mk/prog.mk

