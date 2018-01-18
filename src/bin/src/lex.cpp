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

#include "./lex.hpp"

#include <pn/file>

lexer::lexer(pn::file_view in) { pn_lexer_init(c_obj(), in.c_obj()); }
lexer::~lexer() { pn_lexer_clear(c_obj()); }

void lexer::next(pn_error_t* error) { pn_lexer_next(c_obj(), error); }
