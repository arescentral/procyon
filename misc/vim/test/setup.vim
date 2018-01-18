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

let s:repo = expand('<sfile>:p:h:h')
let &runtimepath = s:repo . "," . &runtimepath
filetype plugin indent on

let s:letters = {
            \ "": " ",
            \ "procyonIndent": "\t",
            \
            \ "procyonNull": "n",
            \ "procyonBool": "b",
            \ "procyonInt": "i",
            \ "procyonFloat": "f",
            \ "procyonInf": "f",
            \ "procyonNan": "f",
            \ "procyonString": "\"",
            \
            \ "procyonDataOp": "$",
            \ "procyonDataBytes": "x",
            \
            \ "procyonStrWrap": ">",
            \ "procyonStrPipe": "|",
            \ "procyonStrBang": "!",
            \ "procyonStrBody": ".",
            \ "procyonEscape": "\\",
            \
            \ "procyonArrayIn": "[",
            \ "procyonArrayOut": "]",
            \ "procyonMapIn": "{",
            \ "procyonMapOut": "}",
            \ "procyonComma": ",",
            \ "procyonMapOp": ":",
            \
            \ "procyonArrayOp": "*",
            \ "procyonShortBareKey": "k",
            \ "procyonShortQuotedKey": "q",
            \ "procyonLongBareKey": "K",
            \ "procyonLongQuotedKey": "Q",
            \
            \ "procyonComment": "#",
            \ "procyonTodo": "X",
            \
            \ "procyonLineError": "E",
            \ "procyonStrError": "E",
            \ "procyonEscapeError": "E",
            \ "procyonArrayError": "E",
            \ "procyonMapError": "E",
            \ "procyonCommaError": "E"}

function! Highlight(filetype, mapping)
    syntax enable
    let &filetype=a:filetype

    let a:lines_in = getline(1, "$")
    let a:lines_out = []
    for row in range(1, len(a:lines_in))
        let a:line_in = a:lines_in[row - 1]
        let a:line_out = ""
        for col in range(1, len(a:line_in))
            let a:line_out .= a:mapping[synIDattr(synID(row, col, 1), "name")]
        endfor
        call add(a:lines_out, a:line_out)
    endfor

    set filetype=
    call setline(1, a:lines_out)
endfunction

command! Highlight :call Highlight("procyon", s:letters)
