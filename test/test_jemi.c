/**
 * @file test_jemi.c
 *
 * MIT License
 *
 * Copyright (c) 2022 R. Dunbar Poor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
To run the tests (on a POSIX / gcc style environment):

gcc -g -Wall -I.. -o test_jemi test_jemi.c ../jemi.c && ./test_jemi && rm -f ./test_jemi

*/

// *****************************************************************************
// Includes

#include "jemi.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

#define JEMI_POOL_SIZE 60
#define MAX_JSON_LENGTH 200

#define ASSERT(e) assert(e, #e, __FILE__, __LINE__)

// *****************************************************************************
// Private (static) storage

static jemi_node_t s_jemi_pool[JEMI_POOL_SIZE];

static char s_json_string[MAX_JSON_LENGTH];

static int s_json_idx;  // index into next char of s_json_string[]

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Print an error message on stdout if expr is false.
 */
static void assert(bool expr, const char *str, const char *file, int line);

/**
 * @brief Write a char at a time into s_json_string[].
 */
static void writer_fn(char c);

/**
 * @brief Render JSON into s_json_string[]
 */
static void test_render(jemi_node_t *node);

// *****************************************************************************
// Public code

int main(void) {
  jemi_node_t *root;

  printf("\nStarting test_jemi...");

  jemi_init(s_jemi_pool, JEMI_POOL_SIZE);
  ASSERT(jemi_available() == JEMI_POOL_SIZE);

  // verify rendinging of each basic type
  jemi_reset();
  root = jemi_object(jemi_object_end());
  test_render(root);
  ASSERT(strcmp(s_json_string, "{}") == 0);

  jemi_reset();
  root = jemi_array(jemi_array_end());
  test_render(root);
  ASSERT(strcmp(s_json_string, "[]") == 0);

  jemi_reset();
  root = jemi_number(1.0);
  test_render(root);
  ASSERT(strcmp(s_json_string, "1.000000") == 0);

  jemi_reset();
  root = jemi_string("red");
  test_render(root);
  ASSERT(strcmp(s_json_string, "\"red\"") == 0);

  jemi_reset();
  root = jemi_bool(true);
  test_render(root);
  ASSERT(strcmp(s_json_string, "true") == 0);

  jemi_reset();
  root = jemi_bool(false);
  test_render(root);
  ASSERT(strcmp(s_json_string, "false") == 0);

  jemi_reset();
  root = jemi_true();
  test_render(root);
  ASSERT(strcmp(s_json_string, "true") == 0);

  jemi_reset();
  root = jemi_false();
  test_render(root);
  ASSERT(strcmp(s_json_string, "false") == 0);

  jemi_reset();
  root = jemi_null();
  test_render(root);
  ASSERT(strcmp(s_json_string, "null") == 0);

  // Test appending elements to an object
  jemi_reset();
  root = jemi_object(jemi_object_end());
  ASSERT(jemi_append_object(root, "red", jemi_number(1)) == root);
  ASSERT(jemi_append_object(root, "grn", jemi_number(2)) == root);
  ASSERT(jemi_append_object(root, "blu", jemi_number(3)) == root);
  test_render(root);
  ASSERT(strcmp(s_json_string, "{\"red\":1.000000,\"grn\":2.000000,\"blu\":3.000000}") == 0);

  // Test appending elements to an array
  jemi_reset();
  root = jemi_array(jemi_array_end());
  ASSERT(jemi_append_array(root, jemi_number(1)) == root);
  ASSERT(jemi_append_array(root, jemi_string("woof")) == root);
  ASSERT(jemi_append_array(root, jemi_true()) == root);
  ASSERT(jemi_append_array(root, jemi_false()) == root);
  ASSERT(jemi_append_array(root, jemi_null()) == root);
  test_render(root);
  ASSERT(strcmp(s_json_string, "[1.000000,\"woof\",true,false,null]") == 0);

  // Test compound object.
  jemi_reset();
  jemi_node_t *obj = jemi_object(jemi_object_end());
  jemi_node_t *arr = jemi_array(jemi_array_end());
  jemi_append_array(arr, jemi_number(1));
  jemi_append_array(arr, jemi_number(2));
  jemi_append_array(arr, jemi_number(3));
  jemi_append_object(obj, "colors", arr);
  test_render(root);
  ASSERT(strcmp(s_json_string, "{\"colors\":[1.000000,2.000000,3.000000]}") == 0);

  // vararg style appending to an array
  jemi_reset();
  test_render(
    jemi_array(
      jemi_number(2),
      jemi_string("moo"),
      jemi_true(),
      jemi_false(),
      jemi_null(),
      jemi_array_end()));
  ASSERT(strcmp(s_json_string, "[2.000000,\"moo\",true,false,null]") == 0);

  // vararg creation of a compound object
  jemi_reset();
  test_render(
    jemi_object(
      jemi_string("colors"),
      jemi_array(
        jemi_number(4),
        jemi_number(5),
        jemi_number(6),
        jemi_array_end()),
      jemi_object_end()));
  ASSERT(strcmp(s_json_string, "{\"colors\":[4.000000,5.000000,6.000000]}") == 0);

  // vararg empty array
  jemi_reset();
  test_render(jemi_array(jemi_array_end()));
  ASSERT(strcmp(s_json_string, "[]") == 0);

  // vararg empty array
  jemi_reset();
  test_render(jemi_object(jemi_object_end()));
  ASSERT(strcmp(s_json_string, "{}") == 0);

  // vararg compound example.
  root =
      jemi_object(
          jemi_string("colors"),
          jemi_array(
              jemi_object(
                  jemi_string("yellow"),
                  jemi_array(jemi_number(255), jemi_number(255), jemi_number(0), jemi_array_end()),
                  jemi_string("cyan"),
                  jemi_array(jemi_number(0), jemi_number(255), jemi_number(255), jemi_array_end()),
                  jemi_string("magenta"),
                  jemi_array(jemi_number(255), jemi_number(0), jemi_number(255), jemi_array_end()),
                  jemi_object_end()),
              jemi_array_end()),
          jemi_object_end());
  test_render(root);
  ASSERT(
      strcmp(
          s_json_string,
          "{\"colors\":[{\"yellow\":[255.000000,255.000000,0.000000],\"cyan\":[0.000000,255.000000,255.000000],\"magenta\":[255.000000,0.000000,255.000000]}]}"
      ) == 0);

  printf("\n... Finished test_jemi\n");
}

// *****************************************************************************
// Private (static) code

static void assert(bool expr, const char *str, const char *file, int line) {
  if (!expr) {
    printf("\nassertion %s failed at %s:%d", str, file, line);
  }
}

static void writer_fn(char c) {
  if (s_json_idx < sizeof(s_json_string)) {
    s_json_string[s_json_idx++] = c;
  }
}

static void test_render(jemi_node_t *node) {
  s_json_idx = 0;
  jemi_emit(node, writer_fn);
  s_json_string[s_json_idx] = '\0';
  printf("\nrendered %s", s_json_string);
}

// *****************************************************************************
// End of file
