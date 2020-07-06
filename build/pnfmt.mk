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

PNFMT := $(OUT)/pnfmt$(EXE_SUFFIX)
PNFMT_SRCS := $(PROCYON)/src/bin/src/pnfmt.cpp
PNFMT_OBJS := $(PNFMT_SRCS:%=$(OUT)/%.o)

$(PNFMT): $(PNFMT_OBJS) $(LIBPROCYON_BIN) $(LIBPROCYON_CPP_A) $(LIBPROCYON_A)
	$(CXX) $+ -o $@ $(LDFLAGS) $(LIBPROCYON_LDFLAGS)

$(PNFMT_OBJS): $(OUT)/%.cpp.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(PNFMT_OBJS:.o=.d)
