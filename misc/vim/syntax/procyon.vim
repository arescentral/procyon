" Vim syntax file
"
" Copyright 2017 The Procyon Authors
"
" Licensed under the Apache License, Version 2.0 (the "License");
" you may not use this file except in compliance with the License.
" You may obtain a copy of the License at
"
"     http://www.apache.org/licenses/LICENSE-2.0
"
" Unless required by applicable law or agreed to in writing, software
" distributed under the License is distributed on an "AS IS" BASIS,
" WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
" See the License for the specific language governing permissions and
" limitations under the License.
"
" Language:	    Procyon
" Maintainer:	Chris Pickel <sfiera@twotaled.com>
" Filenames:	*.pn
" Last Change:	2017-10-13

if exists("b:current_syntax")
  finish
endif

syn case match

" Procyon lines can be highlighted in isolation; use this for speed.
syn sync minlines=1 maxlines=1

"setlocal foldmethod=indent
setlocal iskeyword=A-Z,a-z,43,45-57,_

" Initial {{{
syn match    procyonLine           '^' transparent nextgroup=procyonIndent
syn match    procyonIndent         '[ \t]*' contained transparent nextgroup=@procyonItemL

syn cluster  procyonItemL          contains=procyonWordL,procyonNumberL
syn cluster  procyonItemL          add=procyonDataL,procyonStringL
syn cluster  procyonItemL          add=procyonStrWrap,procyonStrPipe,procyonStrBang
syn cluster  procyonItemL          add=procyonArrayL,procyonMapL
syn cluster  procyonItemL          add=procyonArrayOp,procyonLongBareKey,procyonLongQuotedKey
syn cluster  procyonItemL          add=procyonComment
syn cluster  procyonItemL          add=procyonLineError

syn cluster  procyonItemS          contains=procyonWordL,procyonNumberL
syn cluster  procyonItemS          add=procyonDataL,procyonStringL
syn cluster  procyonItemS          add=procyonArrayL,procyonMapL
syn cluster  procyonItemS          add=procyonLineError
" }}}

" Errors; first so other matches take priority {{{
syn match    procyonLineError         '.\+$' contained
syn match    procyonDataError         '[A-Fa-f0-9 \t]\+' contained
syn match    procyonArrayError        '.[^],\n]*' contained nextgroup=@procyonArrayNext
syn match    procyonArrayCommaError   ',[ \t]*\ze\]' transparent contained contains=procyonCommaError nextgroup=@procyonArrayNext
syn match    procyonMapError          '.[^},\n]*' contained nextgroup=@procyonMapNext
syn match    procyonMapCommaError     ',[ \t]*\ze}' transparent contained contains=procyonCommaError nextgroup=@procyonMapNext
syn match    procyonCommaError        ','
" }}}

" Data {{{
syn match    procyonDataL          '\$[A-Fa-f0-9 \t]*' transparent contained contains=procyonDataOp nextgroup=procyonComment,procyonLineError
syn match    procyonDataA          '\$[A-Fa-f0-9 \t]*' transparent contained contains=procyonDataOp nextgroup=@procyonArrayNext
syn match    procyonDataM          '\$[A-Fa-f0-9 \t]*' transparent contained contains=procyonDataOp nextgroup=@procyonMapNext
syn match    procyonDataOp         '\$' contained nextgroup=@procyonDataNext
syn match    procyonDataBytes      '\([A-Fa-f0-9]\{2\}\)\+' contained nextgroup=@procyonDataNext
syn match    procyonDataSpace      '[ \t]\+' transparent contained nextgroup=@procyonDataNext
syn cluster  procyonDataNext       contains=procyonDataBytes,procyonDataSpace,procyonDataError
" }}}

" Strings {{{
syn match    procyonStringL        '"\([^\\"]\|\\.\)*"\($\|\ze[^: \t]\|[ \t]\+\)' extend transparent contained contains=procyonString nextgroup=procyonComment,procyonLineError
syn match    procyonStringA        '"\([^\\"]\|\\.\)*"\($\|\ze[^: \t]\|[ \t]\+\)' extend transparent contained contains=procyonString nextgroup=@procyonArrayNext
syn match    procyonStringM        '"\([^\\"]\|\\.\)*"\($\|\ze[^: \t]\|[ \t]\+\)' extend transparent contained contains=procyonString nextgroup=@procyonMapNext
syn match    procyonString         '"\([^\\"]\|\\.\)*"' extend contained contains=procyonEscape,procyonEscapeError
syn match    procyonEscapeError    '\\[^"\\/bfnrtu]' contained
syn match    procyonEscapeError    '\\u\x\{0,3}' contained
syn match    procyonEscape         '\\["\\/bfnrt]' contained
syn match    procyonEscape         '\\u\x\{4}' contained

syn match    procyonStrError          '.\+$' contained
syn match    procyonStrBody           '.*$' contained
syn match    procyonStrWrap           '>' contained nextgroup=procyonStrSpace,procyonStrBody
syn match    procyonStrPipe           '|' contained nextgroup=procyonStrSpace,procyonStrBody
syn match    procyonStrBang           '!' contained nextgroup=procyonStrBangSpace,procyonStrError
syn match    procyonStrSpace          '[ \t]' transparent contained nextgroup=procyonStrBody
syn match    procyonStrBangSpace      '[ \t]' transparent contained nextgroup=procyonStrError
" }}}

" Short-form containers {{{
syn region   procyonArrayL         start='\[' end='\][ \t]*' transparent oneline keepend extend contained contains=procyonArrayStart nextgroup=procyonComment,procyonLineError
syn region   procyonArrayA         start='\[' end='\][ \t]*' transparent oneline keepend extend contained contains=procyonArrayStart nextgroup=@procyonArrayNext
syn region   procyonArrayM         start='\[' end='\][ \t]*' transparent oneline keepend extend contained contains=procyonArrayStart nextgroup=@procyonMapNext
syn match    procyonArrayStart     '\[[ \t]*' transparent contained contains=procyonArrayIn nextgroup=@procyonItemA,procyonArrayOut
syn match    procyonArrayIn        '\[' contained
syn match    procyonArrayOut       '\]' contained
syn match    procyonArrayComma     ',[ \t]*\ze[^] \t]' transparent contained contains=procyonComma nextgroup=@procyonItemA
syn cluster  procyonArrayNext      contains=procyonArrayOut,procyonArrayComma,procyonArrayError,procyonArrayCommaError

syn cluster  procyonItemA          contains=procyonWordA,procyonNumberA
syn cluster  procyonItemA          add=procyonDataA,procyonStringA
syn cluster  procyonItemA          add=procyonArrayA,procyonMapA
syn cluster  procyonItemA          add=procyonArrayError

syn region   procyonMapL              start='{' end='}[ \t]*' transparent oneline keepend extend contained contains=procyonMapStart nextgroup=procyonComment,procyonLineError
syn region   procyonMapA              start='{' end='}[ \t]*' transparent oneline keepend extend contained contains=procyonMapStart nextgroup=@procyonArrayNext
syn region   procyonMapM              start='{' end='}[ \t]*' transparent oneline keepend extend contained contains=procyonMapStart nextgroup=@procyonMapNext
syn match    procyonMapStart          '{[ \t]*' transparent contained contains=procyonMapIn nextgroup=procyonShortBareKey,procyonShortQuotedKey,procyonMapOut,procyonMapError
syn match    procyonMapIn             '{' contained
syn match    procyonMapOut            '}' contained
syn match    procyonMapComma          ',[ \t]*\ze\($\|[^} \t]\)' transparent contained contains=procyonComma nextgroup=procyonShortBareKey,procyonShortQuotedKey,procyonMapError
syn match    procyonShortBareKey      '\k*\ze:' contained nextgroup=procyonShortMapOp
syn match    procyonShortQuotedKey    '"\([^\\"]\|\\.\)*"\ze:' contained contains=procyonEscape,procyonEscapeError nextgroup=procyonShortMapOp
syn match    procyonShortMapOp        ':[ \t]*' transparent contained contains=procyonMapOp nextgroup=@procyonItemM
syn cluster  procyonMapNext           contains=procyonMapOut,procyonMapComma,procyonMapError,procyonMapCommaError

syn cluster  procyonItemM          contains=procyonWordM,procyonNumberM
syn cluster  procyonItemM          add=procyonDataM,procyonStringM
syn cluster  procyonItemM          add=procyonArrayM,procyonMapM
syn cluster  procyonItemM          add=procyonMapError

syn match    procyonComma             ',' contained
syn match    procyonMapOp             ':' contained
" }}}

" Long-form containers {{{
syn match    procyonArrayOp           '\*' contained nextgroup=procyonIndent

syn match    procyonLongBareKey       '\k*\ze:' contained nextgroup=procyonLongMapOp
syn match    procyonLongQuotedKey     '"\([^\\"]\|\\.\)*"\ze:' contained contains=procyonEscape,procyonEscapeError nextgroup=procyonLongMapOp
syn match    procyonLongMapOp         ':[ \t]*' transparent contained contains=procyonMapOp nextgroup=@procyonItemS
" }}}

" words (null, true, false, ±inf, nan) {{{
syn match procyonWordL   '\<\(null\|true\|false\|[-+]\?inf\|nan\)\>\($\|\ze[^: \t]\|[ \t]\+\)'
                         \ transparent contained nextgroup=procyonComment,procyonLineError
                         \ contains=procyonNull,procyonBool,procyonInf,procyonNan
syn match procyonWordA   '\<\(null\|true\|false\|[-+]\?inf\|nan\)\>\($\|\ze[^: \t]\|[ \t]\+\)'
                         \ transparent contained nextgroup=@procyonArrayNext
                         \ contains=procyonNull,procyonBool,procyonInf,procyonNan
syn match procyonWordM   '\<\(null\|true\|false\|[-+]\?inf\|nan\)\>\($\|\ze[^: \t]\|[ \t]\+\)'
                         \ transparent contained nextgroup=@procyonMapNext
                         \ contains=procyonNull,procyonBool,procyonInf,procyonNan

syn keyword procyonNull  null contained
syn keyword procyonBool  true false contained
syn keyword procyonInf   inf -inf +inf contained
syn keyword procyonNan   nan contained
" }}}

" numbers {{{
syn match procyonNumberL  '[-+]\?\(0\|[1-9][0-9]*\)\([.][0-9]\+\)\?\([Ee][-+]\?[0-9]\+\)\?\>\($\|\ze[^: \t]\|[ \t]\+\)' transparent contained contains=procyonInt,procyonFloat nextgroup=procyonComment,procyonLineError
syn match procyonNumberA  '[-+]\?\(0\|[1-9][0-9]*\)\([.][0-9]\+\)\?\([Ee][-+]\?[0-9]\+\)\?\>\($\|\ze[^: \t]\|[ \t]\+\)' transparent contained contains=procyonInt,procyonFloat nextgroup=@procyonArrayNext
syn match procyonNumberM  '[-+]\?\(0\|[1-9][0-9]*\)\([.][0-9]\+\)\?\([Ee][-+]\?[0-9]\+\)\?\>\($\|\ze[^: \t]\|[ \t]\+\)' transparent contained contains=procyonInt,procyonFloat nextgroup=@procyonMapNext

syn match procyonInt     '[-+]\?[0-9]\+' contained
syn match procyonFloat   '[-+]\?[0-9]\+[.][0-9]\+' contained
syn match procyonFloat   '[-+]\?[0-9]\+\([.][0-9]\+\)\?[Ee][-+]\?[0-9]*' contained
" }}}

" Comments {{{
syn match    procyonComment        '#.*$' contained contains=procyonTodo
syn keyword  procyonTodo           TODO FIXME XXX contained
" }}}

" Highlighting {{{
hi link  procyonLineError          procyonError
hi link  procyonStrError           procyonError
hi link  procyonEscapeError        procyonError
hi link  procyonDataError          procyonError
hi link  procyonArrayError         procyonError
hi link  procyonMapError           procyonError
hi link  procyonCommaError         procyonError
hi link  procyonError              Error

hi link  procyonNull               Identifier
hi link  procyonBool               Identifier
hi link  procyonInt                Number
hi link  procyonFloat              Float
hi link  procyonInf                Float
hi link  procyonNan                Float

hi link  procyonDataOp             String
hi link  procyonDataBytes          String

hi link  procyonString             String
hi link  procyonEscape             SpecialChar
hi link  procyonStrWrap            Special
hi link  procyonStrPipe            Special
hi link  procyonStrBang            Special
hi link  procyonLongStringBody     Normal

hi link  procyonComment            Comment
hi link  procyonTodo               Todo

hi link  procyonArrayIn            Delimiter
hi link  procyonArrayOut           Delimiter
hi link  procyonMapIn              Delimiter
hi link  procyonMapOut             Delimiter
hi link  procyonComma              Special
hi link  procyonMapOp              Special

hi link  procyonArrayOp            Special
hi link  procyonShortBareKey       procyonBareKey
hi link  procyonShortQuotedKey     procyonQuotedKey
hi link  procyonLongBareKey        procyonBareKey
hi link  procyonLongQuotedKey      procyonQuotedKey
hi link  procyonBareKey            procyonKey
hi link  procyonQuotedKey          procyonKey
hi link  procyonKey                Label
" }}}

let b:current_syntax = "procyon"
