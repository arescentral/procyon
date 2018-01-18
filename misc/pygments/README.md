# Pygments support

The `procyonlexer` package teaches [Pygments][pygments] how to
highlight .pn files. To use it, install the package. Pygments will
detect it as a [plugin][plugin]:

    pip install .
    pygmentize ../../doc/lang.pn | expand -t2

[pygments]: http://pygments.org/
[plugin]: http://pygments.org/docs/plugins/
