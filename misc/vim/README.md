# vim support

This directory is a vim bundle. For instructions on how to install it,
consult your package manager. For example, with [Pathogen][pathogen],
link this directory into your bundles directory:

    $ ln -s ~/$PATH_TO/procyon/misc/vim ~/.vim/bundle/procyon

This bundle includes:

* Syntax highlighting
* An auto-indenter
* Default settings to follow Procyon style
* A [vim-codefmt][codefmt] plugin (add `procyon#InstallCodeFormatter()`
  to your `.vimrc`)

[pathogen]: https://github.com/tpope/vim-pathogen
[codefmt]: https://github.com/google/vim-codefmt
