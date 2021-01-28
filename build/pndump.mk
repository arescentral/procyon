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

PNDUMP := $(OUT)/pndump$(EXE_SUFFIX)
PNDUMP_SRCS := $(PROCYON)/src/bin/src/pndump.cpp
PNDUMP_OBJS := $(PNDUMP_SRCS:%=$(OUT)/%.o)

$(PNDUMP): $(PNDUMP_OBJS) $(LIBPROCYON_BIN) $(LIBPROCYON_CPP) $(LIBPROCYON)
	$(CXX) $+ -o $@ $(LDFLAGS) $(LIBPROCYON_LDFLAGS)

$(PNDUMP_OBJS): $(OUT)/%.cpp.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(PNDUMP_OBJS:.o=.d)
