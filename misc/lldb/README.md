# lldb support

The `pn_lldb.py` script teaches the [lldb debugger][lldb] how to print
Procyon types in C and C++. To start using it, add the following line to
your `~/.lldbinit` with the path to `procyon` on your system:

    command script import ~/$PATH_TO/procyon/misc/lldb/pn_lldb.py

[lldb]: https://lldb.llvm.org/
