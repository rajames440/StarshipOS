PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

# Big endian currently not implemented.
# Missing parts marked with preprocessor errors in code.
SYSTEMS    := x86-l4f amd64-l4f arm-l4f arm64-l4f

TARGET = ahci-drv
SRC_CC = main.cc ahci_device.cc ahci_port.cc hba.cc

REQUIRES_LIBS  := libio-vbus libblock-device

include $(L4DIR)/mk/prog.mk
