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

import io
import json
import math
import os
import procyon
import procyon.lex
import procyon.pn2json
import re
import subprocess
import sys
from .context import procyon, pntest


def jsonify(source):
    try:
        return procyon.pn2json.to_json(io.BytesIO(source)).encode("utf-8"), None
    except procyon.ProcyonDecodeError as e:
        return None, ("%s\n" % e).encode("utf-8")


def flatten(source):
    x = procyon.load(io.StringIO(source.decode("utf-8")))
    if isinstance(x, procyon.py3.unicode):
        return x.encode("utf-8")
    else:
        assert isinstance(x, (bytes, bytearray, memoryview))
        return x


def test_func(run):
    run(reformat=None, jsonify=jsonify, flatten=flatten)


def pytest_generate_tests(metafunc):
    metafunc.parametrize("run", pntest.CASES, ids=pntest.DIRECTORIES)


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
