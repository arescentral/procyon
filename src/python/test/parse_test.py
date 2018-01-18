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

import pytest

try:
    from io import BytesIO, StringIO
except ImportError:
    from cStringIO import StringIO
    BytesIO = StringIO
import sys
from .context import procyon, pntest


def do_parse(source):
    sys.stdin = BytesIO(source)
    sys.stdout = StringIO()
    sys.stderr = StringIO()
    if not procyon.parse.main(["procyon.parse"]):
        out = sys.stdout.getvalue().encode("utf-8")
        err = None
    else:
        out = None
        err = sys.stderr.getvalue().split(": ", 1)[1].encode("utf-8")
    sys.stdin = sys.__stdin__
    sys.stdout = sys.__stdout__
    sys.stderr = sys.__stderr__
    return out, err


def test_func(run):
    run(do_parse)


def pytest_generate_tests(metafunc):
    if metafunc.function == test_func:
        metafunc.parametrize("run", pntest.PARSE_CASES, ids=pntest.DIRECTORIES)


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
