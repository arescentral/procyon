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

LIBPROCYON_CPP_A := $(OUT)/libprocyon-cpp.a
LIBPROCYON_CPP_SRCS := \
	$(PROCYON)/src/cpp/src/array.cpp \
	$(PROCYON)/src/cpp/src/data.cpp \
	$(PROCYON)/src/cpp/src/file.cpp \
	$(PROCYON)/src/cpp/src/map.cpp \
	$(PROCYON)/src/cpp/src/string.cpp \
	$(PROCYON)/src/cpp/src/value.cpp
LIBPROCYON_CPP_OBJS := $(LIBPROCYON_CPP_SRCS:%=$(OUT)/%.o)

$(LIBPROCYON_CPP_A): $(LIBPROCYON_CPP_OBJS)
	$(AR) rcs $@ $+

$(LIBPROCYON_CPP_OBJS): $(OUT)/%.cpp.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIBPROCYON_CPPFLAGS) -c $< -o $@

-include $(LIBPROCYON_CPP_OBJS:.o=.d)
