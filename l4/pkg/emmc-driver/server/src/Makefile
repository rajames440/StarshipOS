PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET  = emmc-drv
SRC_CC  = main.cc \
          cmd.cc \
          drv.cc \
          factory.cc \
          mmc.cc \
          mmio.cc \
          util.cc

SRC_CC-$(CONFIG_EMMC_DRV_SDHCI_BCM2711) += drv_sdhci-bcm2711.cc
SRC_CC-$(CONFIG_EMMC_DRV_SDHCI_USDHC) += drv_sdhci-usdhc.cc
SRC_CC-$(CONFIG_EMMC_DRV_SDHCI_PLAIN) += drv_sdhci-plain.cc
SRC_CC-$(CONFIG_EMMC_DRV_SDHI) += cpg.cc drv_sdhi.cc

REQUIRES_LIBS  := libio-vbus libblock-device libpthread

include $(L4DIR)/mk/prog.mk

#WARNINGS += -Werror
