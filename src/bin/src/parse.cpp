// Copyright 2017 The Procyon Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "./parse.hpp"

#include <exception>
#include <pn/file>

parser::parser(lexer* l, int max_depth) { pn_parser_init(c_obj(), l->c_obj(), max_depth); }
parser::~parser() { pn_parser_clear(c_obj()); }

bool parser::next(pn_error_t* error) {
    using std::swap;
    if (!pn_parser_next(c_obj(), error)) {
        return false;
    } else if (c_obj()->evt.type == PN_EVT_ERROR) {
        throw std::runtime_error(
                pn::format("{0}:{1}: {2}", error->lineno, error->column, pn_strerror(error->code))
                        .c_str());
    }
    return true;
}
