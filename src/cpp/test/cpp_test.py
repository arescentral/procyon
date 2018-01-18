#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
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

from __future__ import absolute_import, division, print_function, unicode_literals

import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
PROCYON_CPP_TEST = os.path.join(ROOT, "out/cur/procyon-cpp-test")

sys.path.insert(0, os.path.join(ROOT, "src", "c", "test"))
import pntest


def test_cpp(flag):
    subprocess.check_call([PROCYON_CPP_TEST, flag])


def pytest_generate_tests(metafunc):
    output = subprocess.check_output([PROCYON_CPP_TEST, "--gtest_list_tests"]).decode("utf-8")
    tests = [x.strip(".") for x in output.splitlines() if x.endswith(".")]
    metafunc.parametrize("flag", ["--gtest_filter=%s.*" % x for x in tests], ids=tests)


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
