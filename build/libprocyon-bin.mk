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

LIBPROCYON_BIN := $(OUT)/libprocyon-bin.a
LIBPROCYON_BIN_SRCS := \
    $(PROCYON)/src/bin/src/lex.cpp \
    $(PROCYON)/src/bin/src/parse.cpp
LIBPROCYON_BIN_OBJS := $(LIBPROCYON_BIN_SRCS:%=$(OUT)/%.o)

$(LIBPROCYON_BIN): $(LIBPROCYON_BIN_OBJS)
	$(AR) rcs $@ $+

$(LIBPROCYON_BIN_OBJS): $(OUT)/%.cpp.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(LIBPROCYON_BIN_OBJS:.o=.d)
