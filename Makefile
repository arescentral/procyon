# Copyright 2017 The Procyon Authors
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

include out/cur/args.gn
NINJA=build/lib/bin/ninja -C out/cur

.PHONY: all
all:
	@$(NINJA)

.PHONY: test
test: test-python3

.PHONY: test-python3
test-python3: all
	python3 -m pytest src misc

.PHONY: test-python2
test-python2:
	python2 -m pytest src/python misc/pygments

.PHONY: test-vim
test-vim:
	misc/vim/test/ftplugin.vroom
	misc/vim/test/indent.vroom
	misc/vim/test/syntax.vroom

.PHONY: regen
regen:
	src/c/scripts/gen.py \
		--c=src/c/src/gen_table.c \
		--python=src/python/procyon/spec.py

.PHONY: clean
clean:
	@$(NINJA) -t clean

.PHONY: distclean
distclean:
	rm -Rf out/

.PHONY: friends
friends:
	@echo "Sure! You can email me at sfiera@sfzmail.com."

.PHONY: love
love:
	@echo "Sorry, I'm not that kind of Makefile."

.PHONY: time
time:
	@echo "I've always got time for you."
