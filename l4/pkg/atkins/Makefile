PKGDIR = .
L4DIR ?= $(PKGDIR)/../..

TARGET = gtest gmock atkins
TARGET_test = selftest

include $(L4DIR)/mk/subdir.mk

selftest: atkins

gmock: gtest
atkins: gtest
