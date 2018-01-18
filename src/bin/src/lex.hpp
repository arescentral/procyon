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

#ifndef PROCYON_LEX_HPP_
#define PROCYON_LEX_HPP_

#include <pn/fwd>

#include "../../c/src/lex.h"

class lexer {
  public:
    using token_t = decltype(pn_lexer_t::token);

    lexer(pn::file_view in);
    ~lexer();

    lexer(const lexer&) = delete;
    lexer& operator=(const lexer&) = delete;

    void next(pn_error_t* error);

    int               lineno() const { return c_obj()->lineno; }
    int               column() const { return token().begin - c_obj()->line.begin + 1; }
    token_t           token() const { return c_obj()->token; }
    pn_lexer_t*       c_obj() { return &_c_obj; }
    const pn_lexer_t* c_obj() const { return &_c_obj; }

  private:
    pn_lexer_t _c_obj;
};

#endif  // PROCYON_LEX_HPP_
