# vim:se ft=make:

include $(L4DIR)/mk/Makeconf

SYSTEMS := amd64-l4f arm64-l4f

CONTRIB_DIR=$(abspath $(PKGDIR)/contrib/ada-$(GNATMAJORVERSION))
