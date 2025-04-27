# vim:se ts=4:
PKGDIR      ?= ../..
L4DIR       ?= $(PKGDIR)/../..

TARGET      = libhistory.a libreadline.a

CONTRIB_INCDIR = readline

PRIVATE_INCDIR = $(PKGDIR)/lib/readline             \
                 $(PKGDIR)/include                  \
                 $(PKGDIR)/lib/contrib/libreadline

WARNINGS    = -Wno-missing-prototypes -Wno-parentheses

DEFINES     = -DHAVE_CONFIG_H -D__L4

#DEBUG      = 1

vpath %     $(PKGDIR)/lib/readline \
            $(PKGDIR)/lib/contrib/libreadline

HISTFILES   = history.c histfile.c histexpand.c histsearch.c xmalloc.c \
              shell.c xfree.c

SRC_C_libhistory.a  = $(HISTFILES)
SRC_C_libreadline.a = readline.c funmap.c keymaps.c vi_mode.c parens.c bind.c \
                      isearch.c display.c util.c kill.c undo.c macro.c input.c \
                      callback.c nls.c savestring.c tilde.c complete.c \
                      $(HISTFILES) rltty.c terminal.c signals.c search.c \
                      replacements.c text.c misc.c mbutil.c colors.c \
                      parse-colors.c compat.c

include $(L4DIR)/mk/lib.mk

# The global include is required because it's not possible to convince gcc to
# skip the current directory when searching for a quoted header. Otherwise we
# had to copy all .c files depending on posixselect.h to lib.
CFLAGS += -Wno-error=implicit-function-declaration \
          -Wno-deprecated-non-prototype \
          -include $(PKGDIR)/lib/readline/posixselect.h
