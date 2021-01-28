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

all:

OUT ?= out/cur
PROCYON ?= .
EXT ?= ext
GMOCK ?= $(EXT)/gmock
MKDIR_P ?= mkdir -p

-include $(OUT)/config.mk

CPPFLAGS += -MMD -MP
CFLAGS += -Wall
CXXFLAGS += -std=c++11 -Wall

include ext/gmock/build/targets.mk
include build/targets.mk

all: lib bin
lib: $(LIBPROCYON) $(LIBPROCYON_CPP) $(PROCYON_CPP_TEST)
bin: $(LIBPROCYON_BIN) $(PN2JSON) $(PNDUMP) $(PNFMT) $(PNPARSE) $(PNTOK)

.PHONY: test
test: test-python

.PHONY: test-python
test-python: all
	python3 -m pytest src misc

.PHONY: test-vim
test-vim:
	misc/vim/test/ftplugin.vroom
	misc/vim/test/indent.vroom
	misc/vim/test/syntax.vroom

.PHONY: test-cpp
test-cpp: $(PROCYON_CPP_TEST)
	$<

.PHONY: test-wine
test-wine: $(PROCYON_CPP_TEST)
	xvfb-run wine64 $<

.PHONY: regen
regen:
	src/c/scripts/gen.py \
		--c=src/c/src/gen_table.c \
		--python=src/python/procyon/spec.py

.PHONY: clean
clean:
	$(RM) -r $(OUT)/

.PHONY: distclean
distclean:
	$(RM) -r out

.PHONY: friends
friends:
	@echo "Sure! You can email me at sfiera@twotaled.com."

.PHONY: love
love:
	@echo "Sorry, I'm not that kind of Makefile."

.PHONY: time
time:
	@echo "I've always got time for you."
