# Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

ifeq ($(OS),Windows_NT)
LDLIBS =
else
LDLIBS = -lrt -lpthread
endif

LDLIBS += -l:libuv.a -lcurl -lcrypto
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o, $(SRCS))
CFLAGS = -Os -Ih -DCURL_STATICLIB
LDFLAGS = -s

# Set environment var ARCH to your architecture to override autodetection.
ifndef ARCH
ifeq ($(findstring x86_64,$(shell $(CC) -dumpmachine)),x86_64)
ARCH = amd64
else
ARCH = x86
endif
endif

ifeq ($(ARCH),amd64)
CFLAGS  += -m64 -D_AMD64_
LDFLAGS += -m64
else
CFLAGS  += -m32
LDFLAGS += -m32
endif

ifeq ($(OS),Windows_NT)
RM = cmd \\\/C del
TARGET = ../bin/highway.exe
LIBDIRS = $(addsuffix /lib,$(addprefix -L,$(wildcard ../3rdparty/*)))
INCDIRS = $(addsuffix /include,$(addprefix -I,$(wildcard ../3rdparty/*)))
LDLIBS += -lws2_32 -lPsapi -liphlpapi -luserenv -lwldap32
else
RM = rm -f
TARGET = highway
endif

LDFLAGS += $(LIBDIRS) $(INCDIRS)
CFLAGS += $(INCDIRS)

$(TARGET):$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include deps

# Make dependencies
deps: $(SRCS)
	$(CC) -MM $(INCDIRS) -Ih $(SRCS) > deps

.PHONY : clean
clean :
	$(RM) deps *.o

