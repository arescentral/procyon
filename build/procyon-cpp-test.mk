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

PROCYON_CPP_TEST := $(OUT)/procyon-cpp-test$(EXE_SUFFIX)
PROCYON_CPP_TEST_SRCS := \
	$(PROCYON)/src/cpp/test/data.test.cpp \
	$(PROCYON)/src/cpp/test/dump.test.cpp \
	$(PROCYON)/src/cpp/test/float.test.cpp \
	$(PROCYON)/src/cpp/test/format.test.cpp \
	$(PROCYON)/src/cpp/test/io.test.cpp \
	$(PROCYON)/src/cpp/test/lex.test.cpp \
	$(PROCYON)/src/cpp/test/matchers.cpp \
	$(PROCYON)/src/cpp/test/parse.test.cpp \
	$(PROCYON)/src/cpp/test/string.test.cpp \
	$(PROCYON)/src/cpp/test/utf8.test.cpp \
	$(PROCYON)/src/cpp/test/value.test.cpp \
	$(PROCYON)/src/cpp/test/valuepp.test.cpp
PROCYON_CPP_TEST_OBJS := $(PROCYON_CPP_TEST_SRCS:%=$(OUT)/%.o)

$(PROCYON_CPP_TEST): $(PROCYON_CPP_TEST_OBJS) $(LIBPROCYON_CPP) $(LIBPROCYON) $(LIBGMOCK_MAIN)
	$(CXX) $+ -o $@ $(LDFLAGS) $(LIBPROCYON_LDFLAGS)

PROCYON_CPP_TEST_CPPFLAGS := \
	$(LIBPROCYON_CPPFLAGS) \
	$(LIBGMOCK_CPPFLAGS) \
	-I src/c/src

$(PROCYON_CPP_TEST_OBJS): $(OUT)/%.cpp.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PROCYON_CPP_TEST_CPPFLAGS) -c $< -o $@

-include $(PROCYON_CPP_TEST_OBJS:.o=.d)
