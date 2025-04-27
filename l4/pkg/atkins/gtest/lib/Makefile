PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

CONTRIB_DIR = $(PKGDIR)/contrib/gmock/googletest

vpath %.cc $(CONTRIB_DIR)/src

TARGET         = libgtest.a libgtest.so
SRC_CC         = gtest-all.cc
CONTRIB_INCDIR = libgtest
PRIVATE_INCDIR = $(CONTRIB_DIR) $(CONTRIB_DIR)/include
PC_FILENAME    = libgtest
PC_CFLAGS      = -DGTEST_OS_L4RE=1
REQUIRES_LIBS  = libstdc++ libpthread

# L4RE_ABS_SOURCE_DIR_PATH="" needed to prevent compile error while
# preprocessing gtest-internal.h
DEFINES        = -DGTEST_OS_L4RE=1 -DL4RE_ABS_SOURCE_DIR_PATH=""
WARNINGS       = $(WARNINGS_MINIMAL)

include $(L4DIR)/mk/lib.mk
