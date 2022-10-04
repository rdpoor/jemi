/**
jemi example.  To compile and run:

gcc -g -Wall -I.. -o example example.c ../jemi.c && ./example && rm -f ./example

jemi_example1() shows how you can use variable length argument to construct a
compound JSON structure with a single top-level call.  The advantage of this
approach is that the nesting of the function calls parallels the nesting of the
JSON structure.

jemi_example2() uses `jemi_append_array()` and `jemi_append_object()` functions
to piecewise build the identical compound JSON structure.  The advantage of this
approach is that you can pre-compute pieces of the JSON and emit the complete
result when you have all of the pieces.

Both examples emit the same JSON string (formatted):
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

 */

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
