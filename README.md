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
              jemi_array(jemi_number(255), jemi_number(255), jemi_number(0), jemi_array_end()),
              jemi_string("cyan"),
              jemi_array(jemi_number(0), jemi_number(255), jemi_number(255), jemi_array_end()),
              jemi_string("magenta"),
              jemi_array(jemi_number(255), jemi_number(0), jemi_number(255), jemi_array_end()),
              jemi_object_end()),
          jemi_object_end());

    jemi_emit(root, (void (*)(char))putchar); // casting avoids compiler warning
    putchar('\n');
}

/**
 * Using the "append" functions to piecewise build a compound JSON structure.
 */
void jemi_example2(void) {
    jemi_node_t *root, *arr, *obj;

    jemi_init(node_pool, JEMI_NODES_MAX);

    // build from the inside out...
    obj = jemi_object(jemi_object_end());
    arr = jemi_array(jemi_array_end());
    jemi_append_array(arr, jemi_number(255));
    jemi_append_array(arr, jemi_number(255));
    jemi_append_array(arr, jemi_number(0));
    jemi_append_object(obj, "yellow", arr);
    arr = jemi_array(jemi_array_end());
    jemi_append_array(arr, jemi_number(0));
    jemi_append_array(arr, jemi_number(255));
    jemi_append_array(arr, jemi_number(255));
    jemi_append_object(obj, "cyan", arr);
    arr = jemi_array(jemi_array_end());
    jemi_append_array(arr, jemi_number(255));
    jemi_append_array(arr, jemi_number(0));
    jemi_append_array(arr, jemi_number(255));
    jemi_append_object(obj, "magenta", arr);

    root = jemi_object(jemi_object_end());
    jemi_append_object(root, "colors", obj);

    jemi_emit(root, (void (*)(char))putchar); // casting avoids compiler warning
    putchar('\n');
}

int main(void) {
    jemi_example1();
    jemi_example2();
}
```
Both `jemi_example1()` and `jemi_example2()` generate the same JSON code which (lightly formatted) looks like this:
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
}```
