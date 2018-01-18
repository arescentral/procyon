" Vim autoload file
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

function! procyon#InstallCodeFormatter() abort
    let l:codefmt_registry = maktaba#extension#GetRegistry('codefmt')
    call l:codefmt_registry.AddExtension(procyon#GetFormatter())
endfunction


""
" @private
" Formatter for Procyon, a object notation.
" Formatter: procyon
function! procyon#GetFormatter() abort
    let l:url = 'https://github.com/arescentral/procyon'
    let l:formatter = {
                \ 'name': 'pnfmt',
                \ 'setup_instructions': 'install Procyon (' . l:url . ')'}

    function l:formatter.IsAvailable() abort
        return executable('pnfmt')
    endfunction

    function l:formatter.AppliesToBuffer() abort
        return &filetype is# 'procyon'
    endfunction

    ""
    " Run `pnfmt` to format the whole file.
    "
    " No FormatRange{,s}(), because pnfmt doesn't provide a hook for formatting a range, and all
    " Procyon files are supposed to be fully formatted anyway.
    function l:formatter.Format() abort
        let l:cmd = [ 'pnfmt' ]
        let l:input = join(getline(1, line('$')), "\n")

        try
            let l:result = maktaba#syscall#Create(l:cmd).WithStdin(l:input).Call(0)
            let l:formatted = split(l:result.stdout, "\n")
            call maktaba#buffer#Overwrite(1, line('$'), l:formatted)
        catch /ERROR(ShellError):/
            let l:errors = []
            for line in split(l:result.stderr, "\n")
                let l:tokens = matchlist(line, '^-:(\d+):(\d+):\s*(.*)')
                if !empty(l:tokens)
                    call add(l:errors, {
                                \ "filename": @%,
                                \ "lnum": l:tokens[1],
                                \ "col": l:tokens[2],
                                \ "text": l:tokens[3]})
                endif
            endfor

            if empty(l:errors)
                " Couldn't parse errors; display the whole error message.
                call maktaba#error#Shout('Error formatting file: %s', l:result.stderr)
            else
                call setqflist(l:errors, 'r')
                cc 1
            endif
        endtry
    endfunction

    return l:formatter
endfunction
