PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= rtc
SRC_CC_x86-l4f   = x86.cc
SRC_CC_amd64-l4f = x86.cc
SRC_CC_arm-l4f   = pl031.cc
SRC_CC_arm64-l4f = pl031.cc
SRC_CC           = main.cc
SRC_CC-$(CONFIG_RTC_DS3231) += ds3231.cc
SRC_CC-$(CONFIG_RTC_PCF85063A) += pcf85063a.cc
REQUIRES_LIBS    = libio cxx_libc_io cxx_io libstdc++

include $(L4DIR)/mk/prog.mk
