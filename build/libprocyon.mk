# Copyright 2020 The Procyon Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LIBPROCYON_CPPFLAGS := -I $(PROCYON)/include
LIBPROCYON_LDFLAGS := -lm -lpthread

ifeq ($(TARGET_OS),win)
LIBPROCYON_LDFLAGS += -lws2_32
endif

LIBPROCYON_A := $(OUT)/libprocyon.a
LIBPROCYON_SRCS := \
	$(PROCYON)/src/c/src/common.c \
	$(PROCYON)/src/c/src/dtoa.c \
	$(PROCYON)/src/c/src/dump.c \
	$(PROCYON)/src/c/src/error.c \
	$(PROCYON)/src/c/src/file.c \
	$(PROCYON)/src/c/src/format.c \
	$(PROCYON)/src/c/src/gen_table.c \
	$(PROCYON)/src/c/src/io.c \
	$(PROCYON)/src/c/src/lex.c \
	$(PROCYON)/src/c/src/numeric.c \
	$(PROCYON)/src/c/src/parse.c \
	$(PROCYON)/src/c/src/procyon.c \
	$(PROCYON)/src/c/src/utf8.c
LIBPROCYON_OBJS := $(LIBPROCYON_SRCS:%=$(OUT)/%.o)

$(LIBPROCYON_A): $(LIBPROCYON_OBJS)
	$(AR) rcs $@ $+

$(LIBPROCYON_OBJS): $(OUT)/%.c.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(LIBPROCYON_OBJS:.o=.d)
