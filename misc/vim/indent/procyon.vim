" Vim indent file
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

if exists("b:did_indent") | finish | endif | let b:did_indent = 1

setlocal nolisp
setlocal autoindent
setlocal indentexpr=GetProcyonIndent(v:lnum)
setlocal indentkeys=o,O

if exists("*GetProcyonIndent") | finish | endif

let s:keepcpo = &cpo
set cpo&vim

function GetProcyonIndent(lnum)
    let prev_lnum = prevnonblank(v:lnum - 1)

    " First non-blank line has zero indent.
    if prev_lnum == 0
        return 0
    endif

    let curr_indent = indent(v:lnum)
    let prev_line = getline(prev_lnum)
    let prev_indent = indent(prev_lnum)

    " Outdenting is always OK.
    if curr_indent < prev_indent
        return -1
    endif

    " Indent by shiftwidth entering array or map.
	let indent = prev_indent
    while prev_line =~ '^\s*[*:]'
		let indent = indent + shiftwidth()
		let prev_line = matchstr(prev_line, '^\s*[*:]\zs.*')
	endwhile

    " Don’t change indent within comment, string, or data.
    " Block colon within comment/string from being recognized as a key.
    if prev_line =~ '^\s*[#>|!$]'
        return indent
    endif

    " Indent by shiftwidth if line ends in key.
	if prev_line =~ ':\S*$'
		let indent = indent + shiftwidth()
	endif

    " Other than arrays or map, don’t indent.
    return indent
endfunction

let &cpo = s:keepcpo
unlet s:keepcpo
