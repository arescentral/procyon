# Procyon <img width="30%" align="right" src="doc/img/logo.svg"/>

Procyon is a simple object notation. It's a little more than
[JSON][json] and a lot less than [YAML][yaml]. For a quick introduction,
see [lang.pn][lang].

[json]: http://json.org
[yaml]: http://yaml.org
[lang]: doc/lang.pn

* [Types](#types)
* [Tools](#types)
  * [pnfmt](#pnfmt)
  * [pn2json](#pn2json)
* [Extras](#extras)

### Repository Structure

| Language  | Source                    |
|-----------|---------------------------|
| C         | [src/c](src/c)            |
| C++       | [src/cpp](src/cpp)        |
| Python    | [src/python](src/python)  |

## Types

There are eight Procyon types:

<table>
  <tr>
    <td>Group</td>
    <td>Subgroup</td>
    <td>Type</td>
    <td>Description</td>
    <td>Short form</td>
  </tr>
  <tr>
    <td rowspan="4">Scalar</td>
    <td rowspan="2">Const</td>
    <td>Null</td>
    <td>Nothing</td>
    <td><tt>null</tt></td>
  </tr>
  <tr>
    <td>Boolean</td>
    <td>True/False</td>
    <td><tt>true</tt>, <tt>false</tt></td>
  </tr>
  <tr>
    <td rowspan="2">Numeric</td>
    <td>Integer</td>
    <td>Signed, 64-bit</td>
    <td><tt>0</tt>, <tt>1</tt>, <tt>-99</tt></td>
  </tr>
  <tr>
    <td>Float</td>
    <td>IEEE 754 double</td>
    <td><tt>0.0</tt>, <tt>1e100</tt>, <tt>-inf</tt>, <tt>nan</tt></td>
  </tr>
  <tr>
    <td rowspan="4">Vector</td>
    <td rowspan="2">Raw</td>
    <td>Data</td>
    <td>Binary data</td>
    <td><tt>$</tt>, <tt>$ff0000</tt></td>
  </tr>
  <tr>
    <td>String</td>
    <td>Unicode</td>
    <td><tt>""</tt>, <tt>"\u270c!"</tt>, <tt>"🍱"</tt></td>
  </tr>
  <tr>
    <td rowspan="2">Sequence</td>
    <td>Array</td>
    <td>Ordered list</td>
    <td><tt>[1, true, 3.0]</tt></td>
  </tr>
  <tr>
    <td>Map</td>
    <td>Key-value pairs</td>
    <td><tt>{b: false, i: 0}</tt></td>
  </tr>
</table>

In addition to short form, the four vector types have a long form:

<table>
  <tr>
    <th>Data</th>
    <th>String</th>
    <th>Array</th>
    <th>Map</th>
  </tr>
  <tr>
    <td valign="top">
      <pre>$ 01234657&#xa;$ 89abcdef&#xa;$ 01234567&#xa;$ 89abcdef</pre>
    </td>
    <td valign="top">
      <pre>&gt; Soft-wrap with “&gt;”&#xa;| Hard-wrap with “|”&#xa;&gt; Final \n unless “!”&#xa!</pre>
    </td>
    <td valign="top">
      <pre>* "Bullets"&#xa;* "Use “*”"</pre>
    </td>
    <td valign="top">
      <pre>key: "unqoted"&#xa;"\"": "quoted"</pre>
    </td>
  </tr>
</table>

## Tools

### pnfmt

Procyon notation has a standard style and format. The `pnfmt` tool will
automatically convert files to the standard style and rewrap strings for
readability.

    usage: pnfmt [-i | -o OUT] [IN]

    options:
     -i, --in-place               format file in-place
     -o, --output=FILE            write output to path
     -h, --help                   show this help screen

### pn2json

pn2json converts Procyon to JSON, with a few conversions for unsupported
types and values:

|Procyon  |JSON    |
|---------|--------|
|`inf`    |`1e999` |
|`nan`    |`null`  |
|`$0123`  |`"0123"`|

    usage: pn2json [options] [FILE.pn]

    options:
         --traditional            format JSON traditionally (default)
         --comma-first            format JSON with comma first
     -m, --minify                 minify JSON
     -r, --root                   print root string or data instead of JSON
     -h, --help                   show this help screen

## Extras

| For       | Source                          |
|-----------|---------------------------------|
| lldb      | [misc/lldb](misc/lldb)          |
| pygments  | [misc/pygments](misc/pygments)  |
| vim       | [misc/vim](misc/vim)            |
