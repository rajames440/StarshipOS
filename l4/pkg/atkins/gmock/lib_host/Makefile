PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

MODE = host

CONTRIB_DIR = $(PKGDIR)/contrib/gmock

vpath %.cc $(CONTRIB_DIR)/googlemock/src

WARNINGS       = $(WARNINGS_MINIMAL)
TARGET         = libgmock.a
SRC_CC         = gmock-all.cc
CONTRIB_INCDIR = libgmock
PC_FILENAME    = libgmock-host
PRIVATE_INCDIR = $(CONTRIB_DIR) \
                 $(CONTRIB_DIR)/googlemock $(CONTRIB_DIR)/googlemock/include \
                 $(CONTRIB_DIR)/googletest $(CONTRIB_DIR)/googletest/include
REQUIRES_LIBS  = libgtest-host

include $(L4DIR)/mk/lib.mk
