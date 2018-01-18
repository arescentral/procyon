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
from .context import PNPARSE, pntest


def pnparse(source):
    p = subprocess.Popen(
        [PNPARSE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate(source)
    if p.returncode == 0:
        return out, None
    else:
        err = err.split(b"pn2json: -:", 1)[-1]
        return None, err


def test_parse(run):
    run(pnparse)


def pytest_generate_tests(metafunc):
    metafunc.parametrize("run", pntest.PARSE_CASES, ids=pntest.DIRECTORIES)


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
