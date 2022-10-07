# jemi
jemi: a compact, pure-C JSON serializer for embedded systems

## Overview

`jemi` makes it easy to construct complex JSON structures and then emit them
to a buffer, a string or a stream.  Designed specifically for embedded systems,
`jemi` is:
* **compact**: one source file and one header file
* **portable**: written in pure C
* **deterministic**: `jemi` uses user-provided data structures and never calls
`malloc()`.
* **yours to use**: `jemi` is covered under the permissive MIT License.

`jemi` derives much of its efficiency and small footprint by a philosophy of
trust: Rather than provide rigorous error checking of input parameters,
`jemi` instead assumes that you provide valid parameters to function calls.


## A Short Example


`jemi` provides two styles for building JSON structures.  The "all in one" style
shown in jemi_example1() lets you use C syntax that parallels the resulting JSON
structure.  The "piecewise" style shown in jemi_example2() shows how you can
pre-build pieces of JSON structure to be emitted later.  Both examples emit the
same results.

```
#include "jemi.h"
#include <stdio.h>

#define JEMI_NODES_MAX 30
static jemi_node_t node_pool[JEMI_NODES_MAX];

/**
 * Using the "all in one" constructors to create a compound JSON structure.
 */
void jemi_example1(void) {
    jemi_init(node_pool, JEMI_NODES_MAX);

    jemi_node_t *root =
        jemi_object(
            jemi_string("colors"),
            jemi_object(
                jemi_string("yellow"),
                jemi_array(jemi_number(255), jemi_number(255), jemi_number(0), NULL),
                jemi_string("cyan"),
                jemi_array(jemi_number(0), jemi_number(255), jemi_number(255), NULL),
                jemi_string("magenta"),
                jemi_array(jemi_number(255), jemi_number(0), jemi_number(255), NULL),
                NULL),
            NULL);
    jemi_emit(root, (void (*)(char))putchar); // casting avoids compiler warning
    putchar('\n');
}

/**
 * Using the "append" functions to piecewise build a compound JSON structure.
 */
void jemi_example2(void) {
    jemi_node_t *root, *dict, *rgb;

    jemi_init(node_pool, JEMI_NODES_MAX);

    // build from the inside out...
    root = jemi_object(NULL);
    jemi_append_object(root, jemi_list(jemi_string("colors"), dict = jemi_object(NULL), NULL));
    jemi_append_object(dict, jemi_list(jemi_string("yellow"), rgb = jemi_array(NULL), NULL));
    jemi_append_array(rgb, jemi_list(jemi_number(255), jemi_number(255), jemi_number(0), NULL));
    jemi_append_object(dict, jemi_list(jemi_string("cyan"), rgb = jemi_array(NULL), NULL));
    jemi_append_array(rgb, jemi_list(jemi_number(0), jemi_number(255), jemi_number(255), NULL));
    jemi_append_object(dict, jemi_list(jemi_string("magenta"), rgb = jemi_array(NULL), NULL));
    jemi_append_array(rgb, jemi_list(jemi_number(255), jemi_number(0), jemi_number(255), NULL));

    jemi_emit(root, (void (*)(char))putchar); // casting avoids compiler warning
    putchar('\n');
}

int main(void) {
    jemi_example1();
    jemi_example2();
}
```

Both `example1()` and `example2()` output the same JSON, which (when formatted
for redability) looks like this:

```
{
   "colors":{
      "yellow":[
         255.000000,
         255.000000,
         0.000000
      ],
      "cyan":[
         0.000000,
         255.000000,
         255.000000
      ],
      "magenta":[
         255.000000,
         0.000000,
         255.000000
      ]
   }
}
```

## Advanced Example

Suppose you know the overall JSON structure that you're going to use, but you
want to dynamically add data to various nodes within the structure before
emitting it.  

jemi handles this case for you with the following features;
* jemi_copy() makes a copy of any jemi structure (or substructure)
* jemi_list() lets you create a "disembodied list" of items that can be
appended to the body of a jemi_list or jemi_object via jemi_append_array()
and jemi_append_object().
* jemi_xxx_set() lets you update the value of a string, boolean or number node.
* The C syntax `a = fn()` evaluates `fn()` and assigns it to `a` and the entire
expression evaluates to that value.  As shown in the example below, you can
exploit this to save references to jemi sub-expressions in the JSON structure.

The result isn't especially pretty, but in practice, you'd create bespoke
functions to fill in and copy your templates and to insert them into your
overall structure.

```
jemi_node_t *snippet_template, *snippets, *color_map, *color_name, *colors,
            *red_val, *grn_val, *blu_val;

// create a snippet that will be used as a key/value pair in an object.
snippet_template =
    jemi_list(
        color_name = jemi_string("color_name"),
        jemi_array(red_val=jemi_number(0),
                   grn_val=jemi_number(0),
                   blu_val=jemi_number(0),
                   jemi_array_end()),
        jemi_list_end());

// Define the overall JSON structure for our color map
color_map =
    jemi_object(
        colors = jemi_string("colors"),
        snippets = jemi_object(jemi_object_end()),
        jemi_object_end());
ASSERT(renders_as(color_map, "{\"colors\":{}}"));

// customize template for yellow and add a copy to the "snippets" sub-structure
jemi_string_set(color_name, "yellow");
jemi_number_set(red_val, 255);
jemi_number_set(grn_val, 255);
jemi_number_set(blu_val, 0);
jemi_append_object(snippets, jemi_copy(snippet_template));
ASSERT(renders_as(color_map, "{\"colors\":{"
                                  "\"yellow\":[255.000000,255.000000,0.000000]"
                                  "}}"));

// customize template for cyan and add a copy to the "snippets" sub-structure
jemi_string_set(color_name, "cyan");
jemi_number_set(red_val, 0);
jemi_number_set(grn_val, 255);
jemi_number_set(blu_val, 255);
jemi_append_object(snippets, jemi_copy(snippet_template));
ASSERT(renders_as(color_map, "{\"colors\":{"
                             "\"yellow\":[255.000000,255.000000,0.000000],"
                             "\"cyan\":[0.000000,255.000000,255.000000]"
                             "}}"));

// customize template for magenta and add a copy to the "snippets" sub-structure
jemi_string_set(color_name, "magenta");
jemi_number_set(red_val, 255);
jemi_number_set(grn_val, 0);
jemi_number_set(blu_val, 255);
jemi_append_object(snippets, jemi_copy(snippet_template));
ASSERT(renders_as(color_map, "{\"colors\":{"
                             "\"yellow\":[255.000000,255.000000,0.000000],"
                             "\"cyan\":[0.000000,255.000000,255.000000],"
                             "\"magenta\":[255.000000,0.000000,255.000000]"
                             "}}"));

```

## No Guard Rails

jemi trusts that you know what you're doing and that you'll pass valid arguments
to the functions.  As such, jemi does very little error checking.  If your
app warrants additional error checking, by all means, write wrappers around the
jemi functions to perform whatever level of safety you need.

With that said, there are a few things to keep in mind:

* Don't forget to pass NULL as the last argument to `jemi_array()`,
`jemi_object()`, or `jemi_list()`.  Failure to do so will almost certainly
result in some sort of segfault.
* Calling `jemi_number_set()` on a node that's not a number will lead to
unexpected results.  Ditto for string and boolean.
* Calling `jemi_append_array()` on a node that's not an array will lead to
unexpected results.  Ditto for `jemi_append_object()`.  However, it is okay
to call `jemi_append_list()` on any jemi node type or NULL.
* The JSON spec requires that each key of a key/value pair in an object is a
string.  However, `jemi_object()` does not enforce this: you can use any jemi
object as a key.  Furthermore, it does not check to see that every key has a
corresponding value.
