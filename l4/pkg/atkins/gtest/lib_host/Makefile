PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

MODE = host

CONTRIB_DIR = $(PKGDIR)/contrib/gmock/googletest

vpath %.cc $(CONTRIB_DIR)/src

TARGET         = libgtest.a
SRC_CC         = gtest-all.cc
CONTRIB_INCDIR = libgtest
PC_FILENAME    = libgtest-host
PRIVATE_INCDIR = $(CONTRIB_DIR) $(CONTRIB_DIR)/include
WARNINGS       = $(WARNINGS_MINIMAL)

include $(L4DIR)/mk/lib.mk
