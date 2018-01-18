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

#ifndef PROCYON_PARSE_HPP_
#define PROCYON_PARSE_HPP_

#include <pn/fwd>

#include "../../c/src/parse.h"
#include "./lex.hpp"

class parser {
  public:
    parser(lexer* l, int max_depth);
    ~parser();

    parser(const parser&) = delete;
    parser& operator=(const parser&) = delete;

    bool next(pn_error_t* error);

    const pn_event_t&  event() { return c_obj()->evt; }
    pn_parser_t*       c_obj() { return &_c_obj; }
    const pn_parser_t* c_obj() const { return &_c_obj; }

  private:
    pn_parser_t _c_obj;
};

#endif  // PROCYON_PARSE_HPP_
