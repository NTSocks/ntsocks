# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2019 Intel Corporation

# binary name
APP = ntb_proxy

VPATH += $(SRCDIR)/utils

# all source are stored in SRCS-y
SRCS-y := main.c
SRCS-y += config.c ntb_mw.c ntp_func.c

SRCS-y += ntm_msg.c ntm_ntp_shm.c ntm_ntp_shmring.c
SRCS-y += ntp_ntm_shm.c ntp_ntm_shmring.c
#SRCS-y += ntp2nts_msg.c ntp2nts_shm.c ntp_nts_shmring.c

#SRCS-y += array.c hash_map.c utils.c



# Build using pkg-config variables if possible
$(shell pkg-config --exists libdpdk)
ifeq ($(.SHELLSTATUS),0)

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)


CFLAGS += -D_FILE_OFFSET_BITS=64 
CFLAGS += -D_BSD_SOURCE 
CFLAGS += -DHAVE_GCC_C11_ATOMICS


PC_FILE := $(shell pkg-config --path libdpdk)
CFLAGS += -O3 $(shell pkg-config --cflags libdpdk)
LDFLAGS_SHARED = $(shell pkg-config --libs libdpdk)
LDFLAGS_STATIC = -Wl,-Bstatic $(shell pkg-config --static --libs libdpdk)

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC) 

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	rmdir --ignore-fail-on-non-empty build

else # Build using legacy build system

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overridden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

ifneq ($(CONFIG_RTE_EXEC_ENV_LINUXAPP),y)
$(info This application can only operate in a linuxapp environment, \
please change the definition of the RTE_TARGET environment variable)
all:
else

VPATH += $(SRCDIR)/utils 

CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -O2
CFLAGS += -D_BSD_SOURCE 
CFLAGS += -DHAVE_GCC_C11_ATOMICS
# CFLAGS += $(WERROR_FLAGS)
# CFLAGS += -DCONFIG_ENABLE_DEBUG
CFLAGS += -DALLOW_EXPERIMENTAL_API
CFLAGS += -pthread  
CFLAGS += -I$(SRCDIR)/include -I/usr/local/include 

LDLIBS += -L/usr/local/lib -lfdu-utils -lpthread


include $(RTE_SDK)/mk/rte.extapp.mk

endif
endif
