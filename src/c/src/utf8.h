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

#ifndef PROCYON_UTF8_H_
#define PROCYON_UTF8_H_

#include <pn/procyon.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef uint32_t pn_rune_t;

void   pn_ascchr(uint8_t rune, char* data, size_t* size);
void   pn_unichr(pn_rune_t rune, char* data, size_t* size);
size_t pn_rune_width(pn_rune_t rune);
size_t pn_str_width(const char* data, size_t size);

bool pn_isascii(pn_rune_t r);
bool pn_isrune(pn_rune_t r);

bool pn_isalnum(pn_rune_t r);    // Abc123あいう英美四㊀㊁㊂
bool pn_isalpha(pn_rune_t r);    // Abcあいう英美四
bool pn_iscntrl(pn_rune_t r);    // \0\n\t\x9f
bool pn_isdigit(pn_rune_t r);    // 123۱۲۳𝟙𝟚𝟛
bool pn_islower(pn_rune_t r);    // abcáḅçａｂｃ
bool pn_isnumeric(pn_rune_t r);  // 123۱۲۳½⅔¾㊀㊁㊂
bool pn_isprint(pn_rune_t r);    // A$ :)
bool pn_ispunct(pn_rune_t r);    // 「(+±-〜:)」
bool pn_isspace(pn_rune_t r);    // \x20\u3000
bool pn_isupper(pn_rune_t r);    // ABCÁḄÇＡＢＣ
bool pn_istitle(pn_rune_t r);    // ǅᾼ

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // PROCYON_UTF8_H_
