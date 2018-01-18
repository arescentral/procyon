Comparison of Formats
=====================

Overview
--------

|Format    |Procyon   |JSON      |YAML      |TOML      |msgpack   |
|----------|----------|----------|----------|----------|----------|
|Structure |Indented  |Braced    |Indented  |INI       |Binary    |
|Comments  |yes       |no        |yes       |yes       |no        |
|References|no        |no        |yes       |          |no        |

Types
-----

|Format    |Procyon   |JSON      |YAML      |TOML      |msgpack   |
|----------|----------|----------|----------|----------|----------|
|Null      |null      |null      |null      |          |nil       |
|Boolean   |bool      |boolean   |bool      |boolean   |bool      |
|Integral  |int       |          |int       |integer   |int       |
|Real      |float     |number    |float     |          |float     |
|Date/Time |          |          |timestamp |date+time |timestamp |
|Binary    |data      |          |binary    |          |bin       |
|Unicode   |string    |string    |str       |string    |str       |
|Array     |array     |array     |seq       |array     |array     |
|Map       |map       |object    |map       |table     |map       |
|User      |          |          |yes       |          |ext       |

<table style="width:100%;table-layout:fixed">
  <tr>
    <th>Format</th>
    <th>Procyon</th>
    <th>JSON</th>
    <th>YAML</th>
    <th>TOML</th>
    <th>msgpack</th>
  </tr>
  <tr>
    <td>Null</td>
    <td><tt>null</tt></td>
    <td><tt>null</tt></td>
    <td><i>blank</i></td>
    <td><i>N/A</i></td>
    <td><tt>$c0</tt></td>
  </tr>
  <tr>
    <td>True</td>
    <td><tt>true</tt></td>
    <td><tt>true</tt></td>
    <td><tt>true</tt></td>
    <td><tt>true</tt></td>
    <td><tt>$c3</tt></td>
  </tr>
  <tr>
    <td>1×10¹⁰⁰</td>
    <td><tt>1e100</tt></td>
    <td><tt>1e100</tt></td>
    <td><tt>1e100</tt></td>
    <td><tt>1e100</tt></td>
    <td><tt>$cf00 0000 0000 0000 00</tt></td>
  </tr>
  <tr>
    <td>$0000</td>
    <td><tt>$0000</tt></td>
    <td><i>N/A</i></td>
    <td><tt>!!binary AAA=</tt></td>
    <td><i>N/A</i></td>
    <td><tt>$c4020000</tt></td>
  </tr>
  <tr>
    <td>🍱</td>
    <td><tt>"\U0001f371"</tt></td>
    <td><tt>"\ud83c\udf71"</tt></td>
    <td><tt>"\U0001F371"</tt></td>
    <td><tt>"\U0001f371"</tt></td>
    <td><tt>$a6 f09f 8db1</tt></td>
  </tr>
</table>

Examples
--------

<table style="width:100%;table-layout:fixed">
  <tr>
    <th>Procyon</th>
    <th>JSON</th>
  </tr>
  <tr>
    <td><pre>fruit:&#xa;  * name:  "apple"&#xa;    physical:&#xa;      color:  "red"&#xa;      shape:  "round"&#xa;    variety:&#xa;      * name:  "red delicious"&#xa;      * name:  "granny smith"&#xa;
  * name:  "banana"&#xa;    variety:&#xa;      * name:  "plantain"</pre></td>
    <td><pre>{&#xa;  "fruit": [{&#xa;    "name": "apple",&#xa;    "physical": {&#xa;      "color": "red",&#xa;      "shape": "round"&#xa;    },&#xa;    "variety": [&#xa;      { "name": "red delicious" },&#xa;      { "name": "granny smith" }&#xa;    ]&#xa;  }, {&#xa;    "name": "banana",&#xa;    "variety": [&#xa;      { "name": "plantain" }&#xa;    ]&#xa;  }]&#xa;}</pre></td>
  </tr>
  <tr>
    <th>YAML</th>
    <th>TOML</th>
  </tr>
  <tr>
    <td><pre>fruit:&#xa;- name: apple&#xa;  physical:&#xa;    color: red&#xa;    shape: round&#xa;  variety:&#xa;  - {name: red delicious}&#xa;  - {name: granny smith}&#xa;&#xa;- name: banana&#xa;  variety:&#xa;  - {name: plantain}</pre></td>
    <td><pre>[[fruit]]&#xa;  name = "apple"&#xa;&#xa;  [fruit.physical]&#xa;    color = "red"&#xa;    shape = "round"&#xa;&#xa;  [[fruit.variety]]&#xa;    name = "red delicious"&#xa;&#xa;  [[fruit.variety]]&#xa;    name = "granny smith"&#xa;&#xa;[[fruit]]&#xa;  name = "banana"&#xa;&#xa;  [[fruit.variety]]&#xa;    name = "plantain"</pre></td>
  </tr>
  <tr>
    <th>Plist</th>
    <th>msgpack</th>
  </tr>
  <tr>
    <td><pre>&lt;plist version="1.0"&gt;&#xa;&lt;array&gt;&#xa;  &lt;dict&gt;&#xa;    &lt;key>name&lt;/key&gt;&#xa;    &lt;string>apple&lt;/string&gt;&#xa;    &lt;key>physical&lt;/key&gt;&#xa;    &lt;dict&gt;&#xa;      &lt;key>color&lt;/key&gt;&#xa;      &lt;string>red&lt;/string&gt;&#xa;      &lt;key>shape&lt;/key&gt;&#xa;      &lt;string>round&lt;/string&gt;&#xa;    &lt;/dict&gt;&#xa;    &lt;key>variety&lt;/key&gt;&#xa;    &lt;array&gt;&#xa;      &lt;dict&gt;&#xa;        &lt;key>name&lt;/key&gt;&#xa;        &lt;string>red delicious&lt;/string&gt;&#xa;      &lt;/dict&gt;&#xa;      &lt;dict&gt;&#xa;        &lt;key>name&lt;/key&gt;&#xa;        &lt;string>granny smith&lt;/string&gt;&#xa;      &lt;/dict&gt;&#xa;    &lt;/array&gt;&#xa;  &lt;/dict&gt;&#xa;  &lt;dict&gt;&#xa;    &lt;key>name&lt;/key&gt;&#xa;    &lt;string>banana&lt;/string&gt;&#xa;    &lt;key>variety&lt;/key&gt;&#xa;    &lt;array&gt;&#xa;      &lt;dict&gt;&#xa;        &lt;key>name&lt;/key&gt;&#xa;        &lt;string>plantain&lt;/string&gt;&#xa;      &lt;/dict&gt;&#xa;    &lt;/array&gt;&#xa;  &lt;/dict&gt;&#xa;&lt;/array&gt;&#xa;&lt;/plist></pre></td>
    <td><pre>$81&#xa;  $a5 "fruit" $92&#xa;    $83&#xa;      $a4 "name" $a5 "apple"&#xa;      $a8 "physical" $82&#xa;        $a5 "color" $a3 "red&#xa;        $a5 "shape" $a5 "round"&#xa;      $a7 "variety" $92&#xa;        $81&#xa;          $a4 "name" $ad "red delicious"&#xa;        $81&#xa;          $a4 "name" $ac "granny smith"&#xa;    $82&#xa;      $a4 "name" $a6 "banana"&#xa;      $a7 "variety" $91&#xa;        $81&#xa;      	  $a4 "name" $a8 "plantain"</pre></td>
  </tr>
</table>
