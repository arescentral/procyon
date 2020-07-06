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

PNPARSE := $(OUT)/pnparse$(EXE_SUFFIX)
PNPARSE_SRCS := $(PROCYON)/src/bin/src/pnparse.c
PNPARSE_OBJS := $(PNPARSE_SRCS:%=$(OUT)/%.o)

$(PNPARSE): $(PNPARSE_OBJS) $(LIBPROCYON_A)
	$(CC) $+ -o $@ $(LDFLAGS) $(LIBPROCYON_LDFLAGS)

$(PNPARSE_OBJS): $(OUT)/%.c.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(PNPARSE_OBJS:.o=.d)
