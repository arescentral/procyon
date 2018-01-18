#!/usr/bin/env python
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
"""Procyon formatters for lldb(1)

Load them in ~/.lldbinit or a running session with this command:
    command script import $PROCYON/misc/lldb/procyon.py
"""

from __future__ import absolute_import, division, print_function

import functools
import lldb
import struct
import unicodedata

SPECIAL_CHARS = {
    u"\b": "\\b",
    u"\f": "\\f",
    u"\n": "\\n",
    u"\r": "\\r",
    u"\t": "\\t",
    u"\"": "\\\"",
    u"\\": "\\\\",
    u" ": u" ",
    u"\u3000": u"\u3000",
}

summarizers = []
summary = lambda t: lambda f: (summarizers.append((t, f)), f)[1]


def join(f):
    """Calls "".join() on returned sequence."""
    return functools.update_wrapper(lambda *args, **kwds: "".join(f(*args, **kwds)), f)


def values(x):
    """Given x:SBValue, returns x.values:SBValue as sized array."""
    count = x.GetChildMemberWithName("count").GetValueAsUnsigned(0)
    v = x.GetChildMemberWithName("values")
    return v.Cast(v.GetType().GetArrayElementType().GetArrayType(count))


def code_points(s):
    """Yields (char:unicode, cp:int) from s:unicode."""
    utf32be = s.encode("utf-32be")
    for i in xrange(0, len(utf32be), 4):
        s = utf32be[i:i + 4]
        yield s.decode("utf-32be"), struct.unpack('>L', s)[0]


@join
def summarize_data(data):
    error = lldb.SBError()
    yield "$"
    if data.GetByteSize() > 0:
        for byte in data.ReadRawData(error, 0, data.GetByteSize()):
            yield "%02x" % ord(byte)


@join
def summarize_string(data):
    error = lldb.SBError()
    yield '"'
    if data.GetByteSize() > 0:
        s = data.ReadRawData(error, 0, data.GetByteSize()).decode("utf-8", errors="replace")
        for char, cp in code_points(s):
            category = unicodedata.category(char)
            if char in SPECIAL_CHARS:
                yield SPECIAL_CHARS[char]
            elif not (category.startswith("C") or category.startswith("Z")):
                yield char.encode("utf-8")
            elif cp < 0x10000:
                yield "\\u%04x" % cp
            else:
                yield "\\U%08x" % cp
    yield '"'


@join
def summarize_array(values):
    yield "["
    for i in xrange(values.GetNumChildren()):
        if i > 0:
            yield ", "
        yield summarize_c_pn_value(values.GetChildAtIndex(i), None)
    yield "]"


@join
def summarize_map(values):
    yield "{"
    for i in xrange(values.GetNumChildren()):
        if i > 0:
            yield ", "
        yield summarize_c_pn_kv_pair(values.GetChildAtIndex(i), None)
    yield "}"


@summary("pn_value")
def summarize_c_pn_value(valobj, unused):
    pn_type = valobj.GetChildMemberWithName("type").GetValueAsUnsigned(0)
    if pn_type == 0:
        return "null"
    elif pn_type == 1:
        b = valobj.GetChildMemberWithName("b").GetValueAsUnsigned(0)
        if b:
            return "true"
        else:
            return "false"
    elif pn_type == 2:
        i = valobj.GetChildMemberWithName("i").GetValueAsSigned(0)
        return str(i)
    elif pn_type == 3:
        error = lldb.SBError()
        f = valobj.GetChildMemberWithName("f").GetData().GetDouble(error, 0)
        return str(f)
    elif pn_type == 4:
        return summarize_c_pn_data(valobj.GetChildMemberWithName("d").Dereference(), unused)
    elif pn_type == 5:
        return summarize_c_pn_string(valobj.GetChildMemberWithName("s").Dereference(), unused)
    elif pn_type == 6:
        return summarize_c_pn_array(valobj.GetChildMemberWithName("a").Dereference(), unused)
    elif pn_type == 7:
        return summarize_c_pn_map(valobj.GetChildMemberWithName("m").Dereference(), unused)
    else:
        return "(bad)"


@summary("pn_data")
def summarize_c_pn_data(valobj, unused):
    count = valobj.GetChildMemberWithName("count").GetValueAsUnsigned(0)
    values = valobj.GetChildMemberWithName("values").AddressOf().GetPointeeData(0, count)
    return summarize_data(values)


@summary("pn_string")
def summarize_c_pn_string(valobj, unused):
    count = valobj.GetChildMemberWithName("count").GetValueAsUnsigned(0) - 1
    values = valobj.GetChildMemberWithName("values").AddressOf().GetPointeeData(0, count)
    return summarize_string(values)


@summary("pn_array")
def summarize_c_pn_array(valobj, unused):
    return summarize_array(values(valobj))


@summary("pn_kv_pair")
def summarize_c_pn_kv_pair(valobj, unused):
    key = valobj.GetChildMemberWithName("key").Dereference()
    value = valobj.GetChildMemberWithName("value")
    return "%s: %s" % (summarize_c_pn_string(key, None), summarize_c_pn_value(value, None))


@summary("pn_map")
def summarize_c_pn_map(valobj, unused):
    return summarize_map(values(valobj))


@summary("pn::value")
@summary("pn::value_ref")
@summary("pn::value_cref")
def summarize_cpp_pn_value(valobj, unused):
    return summarize_c_pn_value(valobj.GetChildMemberWithName("_c_obj"), unused)


@summary("pn::data")
@summary("pn::data_ref")
def summarize_cpp_pn_data(valobj, unused):
    return summarize_c_pn_data(valobj.GetChildMemberWithName("_c_obj").Dereference(), unused)


@summary("pn::data_view")
def summarize_cpp_pn_data_view(valobj, unused):
    count = valobj.GetChildMemberWithName("_size").GetValueAsUnsigned(0)
    values = valobj.GetChildMemberWithName("_data").GetPointeeData(0, count)
    return summarize_data(values)


@summary("pn::string")
@summary("pn::string_ref")
def summarize_cpp_pn_string(valobj, unused):
    return summarize_c_pn_string(valobj.GetChildMemberWithName("_c_obj").Dereference(), unused)


@summary("pn::string_view")
def summarize_cpp_pn_string_view(valobj, unused):
    count = valobj.GetChildMemberWithName("_size").GetValueAsUnsigned(0)
    values = valobj.GetChildMemberWithName("_data").GetPointeeData(0, count)
    return summarize_string(values)


@summary("pn::array")
@summary("pn::array_ref")
@summary("pn::array_cref")
def summarize_cpp_pn_array(valobj, unused):
    return summarize_c_pn_array(valobj.GetChildMemberWithName("_c_obj").Dereference(), unused)


@summary("pn::map")
@summary("pn::map_ref")
@summary("pn::map_cref")
def summarize_cpp_pn_map(valobj, unused):
    return summarize_c_pn_map(valobj.GetChildMemberWithName("_c_obj").Dereference(), unused)


def __lldb_init_module(debugger, unused):
    for t, func in summarizers:
        debugger.HandleCommand(
            "type summary add -w procyon -F procyon.{0} {1}".format(func.__name__, t))
    debugger.HandleCommand("type category enable procyon")
