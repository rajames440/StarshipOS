PKGDIR	?= .
L4DIR	?= $(PKGDIR)/../../..

include $(L4DIR)/mk/Makeconf

verinfo=($1=$($1), Major version: $(if $2,$2,not found))
adac_verinfo=$(call verinfo,ADAC,$(GNATMAJORVERSION))
hadac_verinfo=$(call verinfo,HOST_ADAC,$(HOST_GNATMAJORVERSION))

supported_versions=$(patsubst libgnat-%,%,$(wildcard libgnat-*))
ifeq ($(BID_COMPILER_IS_GCC),)
  $(info $(shell echo -e '\033[32mada only supported with GCC, not with clang\033[0m'))
else ifneq ($(GNATMAJORVERSION),$(HOST_GNATMAJORVERSION))
  $(info $(shell echo -e '\033[32mhost gnatmake $(hadac_verinfo) not available in same native version as target compiler $(adac_verinfo). This is required for building libgnat.\033[0m'))
else ifeq ($(GNATMAJORVERSION),)
    $(info $(shell echo -e '\033[32mlibgnat requires the gnatmake ($(ADAC)) compiler to be installed\033[0m'))
else ifeq ($(wildcard libgnat-$(GNATMAJORVERSION)),)
    $(info $(shell echo -e '\033[32mVersion of gnatmake $(adac_verinfo) not supported. Supported: $(supported_versions)\033[0m'))
else
  TARGET := adainclude libgnat-$(GNATMAJORVERSION) xoscons xsnamest
endif

include $(L4DIR)/mk/subdir.mk

adainclude: xoscons xsnamest
libgnat-$(GNATMAJORVERSION): adainclude

