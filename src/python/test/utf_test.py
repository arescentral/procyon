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

from .context import procyon


def test_has_surrogates():
    assert not procyon.utf.has_surrogates("")
    assert not procyon.utf.has_surrogates("no surrogates here")

    assert not procyon.utf.has_surrogates("\U00100001")
    assert not procyon.utf.has_surrogates("\U0010FFFF")

    assert procyon.utf.has_surrogates("\uD800")
    assert procyon.utf.has_surrogates("\uDC00")
    assert procyon.utf.has_surrogates("\uDFFF")


if __name__ == "__main__":
    import pytest
    raise SystemExit(pytest.main())
