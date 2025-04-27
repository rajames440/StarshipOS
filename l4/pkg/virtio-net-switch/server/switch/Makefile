PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..

TARGET          = l4vio_switch

REQUIRES_LIBS   = libstdc++ l4virtio
REQUIRES_LIBS-$(CONFIG_VNS_IXL) += ixl

SRC_CC-$(CONFIG_VNS_PORT_FILTER) += filter.cc

SRC_CC = main.cc switch.cc options.cc

include $(L4DIR)/mk/prog.mk
