" Vim ftplugin file
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

if exists("b:did_ftplugin")
	finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

" sts=-1 and sw=0 are not available in older versions of vim. They
" should really be conditionally set to 2 on version that don't support
" them.
setlocal noexpandtab tabstop=2 softtabstop=-1 shiftwidth=0
setlocal comments=:#,:>,:\| commentstring=#\ %s
setlocal formatoptions-=t formatoptions+=croqlj

let &cpo = s:cpo_save
unlet s:cpo_save
