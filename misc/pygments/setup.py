import setuptools

setuptools.setup(
    name="procyonlexer",
    url="https://github.com/arescentral/procyon/tree/master/misc/pygments",
    version="1.0",
    py_modules=["procyonlexer"],
    install_requires=[
        "pygments",  # Not sure what version is necessary or sufficient.
    ],
    entry_points={
        "pygments.lexers": [
            "procyon=procyonlexer:ProcyonLexer",
        ],
    },
    author="Chris Pickel",
    author_email="sfiera@twotaled.com",
    description="Adds lexer for Procyon (.pn) files",
    license="Apache Software License",
    keywords="procyon pn pygments plugin",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "Topic :: Text Processing :: Filters",
        "Topic :: Text Processing :: Markup",
        "Topic :: Utilities",
    ], )
