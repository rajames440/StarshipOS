PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET  = jvm_server
SRC_CC  = main.cc

# Canonical location of your unpacked OpenJDK runtime (from jdk.tar.gz)
JDK_HOME = $(L4DIR)/target/jdk

# Add include paths for JNI and platform-specific headers
CFLAGS += -I$(JDK_HOME)/include -I$(JDK_HOME)/include/linux

# Link statically against Java Launcher Interface if available
LDADD += $(JDK_HOME)/lib/jli/libjli.a

# Add system libs if you get unresolved dlopen/pthread symbols
LDADD += -ldl -lpthread

include $(L4DIR)/mk/prog.mk
