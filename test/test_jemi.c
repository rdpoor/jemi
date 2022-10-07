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

gcc -g -Wall -I.. -o test_jemi test_jemi.c ../jemi.c && ./test_jemi && rm -rf ./test_jemi ./test_jemi.dSYM

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

typedef struct {
    char *buf;
    size_t buflen;
    size_t index;
} json_writer_ctx;


// *****************************************************************************
// Private (static) storage

static jemi_node_t s_jemi_pool[JEMI_POOL_SIZE];

static char s_json_string[MAX_JSON_LENGTH];

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Print an error message on stdout if expr is false.
 */
static void assert(bool expr, const char *str, const char *file, int line);

/**
 * @brief Write a char at a time into s_json_string[].
 */
static void writer_fn(char c, void *ctx);

/**
 * @brief Render JSON and compare against expected
 */
static bool renders_as(jemi_node_t *node, const char *expected);

// *****************************************************************************
// Public code

int main(void) {
    jemi_node_t *root;

    printf("\nStarting test_jemi...");

    // jemi_init() initializes the free list
    jemi_init(s_jemi_pool, JEMI_POOL_SIZE);
    ASSERT(jemi_available() == JEMI_POOL_SIZE);

    // jemi_available() returns number of available nodes
    for (int i=0; i<JEMI_POOL_SIZE; i++) {
      ASSERT(jemi_available() == JEMI_POOL_SIZE - i);
      ASSERT(jemi_true() != NULL);
    }

    // Allocation with empty freelist returns NULL
    ASSERT(jemi_available() == 0);
    ASSERT(jemi_true() == NULL);

    // jemi_reset() resets the freelist
    jemi_reset();
    ASSERT(jemi_available() == JEMI_POOL_SIZE);

    // verify creation and rendering of each basic type
    jemi_reset();
    root = jemi_array(NULL);
    ASSERT(renders_as(root, "[]"));

    jemi_reset();
    root = jemi_object(NULL);
    ASSERT(renders_as(root, "{}"));

    jemi_reset();
    root = jemi_list(NULL);
    ASSERT(renders_as(root, ""));

    jemi_reset();
    root = jemi_float(1.0);
    ASSERT(renders_as(root, "1.000000"));

    jemi_reset();
    root = jemi_integer(1);
    ASSERT(renders_as(root, "1"));

    jemi_reset();
    root = jemi_integer(INT64_MAX);  // most positive
    ASSERT(renders_as(root, "9223372036854775807"));

    jemi_reset();
    root = jemi_integer(INT64_MIN);  // most negative
    ASSERT(renders_as(root, "-9223372036854775808"));

    jemi_reset();
    root = jemi_string("red");
    ASSERT(renders_as(root, "\"red\""));

    jemi_reset();
    root = jemi_bool(true);
    ASSERT(renders_as(root, "true"));

    jemi_reset();
    root = jemi_bool(false);
    ASSERT(renders_as(root, "false"));

    jemi_reset();
    root = jemi_true();
    ASSERT(renders_as(root, "true"));

    jemi_reset();
    root = jemi_false();
    ASSERT(renders_as(root, "false"));

    jemi_reset();
    root = jemi_null();
    ASSERT(renders_as(root, "null"));

    // Adding key-value pairs
    jemi_reset();
    root = jemi_object(NULL);
    jemi_object_add_keyval(root, "color", jemi_string("white"));
    jemi_object_add_keyval(root, "on sale", jemi_true());
    jemi_object_add_keyval(root, "weight", jemi_float(1.23));
    jemi_object_add_keyval(root, "sku", jemi_integer(98765));
    ASSERT(renders_as(root, "{"
                            "\"color\":\"white\","
                            "\"on sale\":true,"
                            "\"weight\":1.230000,"
                            "\"sku\":98765"
                            "}"));

    // Compose compound structures "top down" with jemi_object(), jemi_array()
    jemi_reset();
    root = jemi_object(
        jemi_string("colors"),
        jemi_object(
            jemi_string("yellow"),
            jemi_array(jemi_integer(255), jemi_integer(255), jemi_integer(0), NULL),
            jemi_string("cyan"),
            jemi_array(jemi_integer(0), jemi_integer(255), jemi_integer(255), NULL),
            jemi_string("magenta"),
            jemi_array(jemi_integer(255), jemi_integer(0), jemi_integer(255), NULL),
            NULL),
        NULL);
    ASSERT(renders_as(root, "{\"colors\":{"
                            "\"yellow\":[255,255,0],"
                            "\"cyan\":[0,255,255],"
                            "\"magenta\":[255,0,255]"
                            "}}"));

    // Compose compound structures from "bottom up"
    jemi_reset();
    root = jemi_object(NULL);
    do {
        jemi_node_t *dict, *rgb;
        jemi_append_object(root, jemi_list(jemi_string("colors"), dict = jemi_object(NULL), NULL));
        jemi_append_object(dict, jemi_list(jemi_string("yellow"), rgb = jemi_array(NULL), NULL));
        jemi_append_array(rgb, jemi_list(jemi_integer(255), jemi_integer(255), jemi_integer(0), NULL));
        jemi_append_object(dict, jemi_list(jemi_string("cyan"), rgb = jemi_array(NULL), NULL));
        jemi_append_array(rgb, jemi_list(jemi_integer(0), jemi_integer(255), jemi_integer(255), NULL));
        jemi_append_object(dict, jemi_list(jemi_string("magenta"), rgb = jemi_array(NULL), NULL));
        jemi_append_array(rgb, jemi_list(jemi_integer(255), jemi_integer(0), jemi_integer(255), NULL));
    } while(false);
    ASSERT(renders_as(root, "{\"colors\":{"
                            "\"yellow\":[255,255,0],"
                            "\"cyan\":[0,255,255],"
                            "\"magenta\":[255,0,255]"
                            "}}"));

    // jemi_list() creates "disembodied" lists
    jemi_reset();
    root = jemi_list(jemi_true(), NULL);
    ASSERT(renders_as(root, "true"));

    jemi_reset();
    root = jemi_list(jemi_true(), jemi_false(), NULL);
    ASSERT(renders_as(root, "true,false"));

    // jemi_copy() makes a copy of a jemi_node structure
    jemi_reset();
    root = jemi_array(jemi_float(1), jemi_string("woof"), NULL);
    ASSERT(renders_as(root, "[1.000000,\"woof\"]"));
    do {
        jemi_node_t *alt = jemi_copy(root);
        ASSERT(alt != root);
        ASSERT(renders_as(alt, "[1.000000,\"woof\"]"));
    } while(false);

    // Use jemi_copy() and jemi_list() to create templates
    //
    // Recall that in C, the expression "a = b()" evaluates to b().
    // We exploit this to create a template structure with references to
    // the fields we want to update.
    //
    do {
        jemi_node_t *snippet_template, *snippets, *color_map, *color_name, *colors, *red_val, *grn_val, *blu_val;

        // create a snippit that will be used as a key/value pair in an object.
        snippet_template =
            jemi_list(
                color_name = jemi_string("color_name"),
                jemi_array(red_val=jemi_integer(0),
                           grn_val=jemi_integer(0),
                           blu_val=jemi_integer(0),
                           NULL),
                NULL);

        // Define the overall JSON structure for our color map
        color_map =
            jemi_object(
                colors = jemi_string("colors"),
                snippets = jemi_object(NULL),
                NULL);
        ASSERT(renders_as(color_map, "{\"colors\":{"
                                     "}}"));

        // customize template for yellow and add a copy to the JSON structure
        jemi_string_set(color_name, "yellow");
        jemi_integer_set(red_val, 255);
        jemi_integer_set(grn_val, 255);
        jemi_integer_set(blu_val, 0);
        jemi_append_object(snippets, jemi_copy(snippet_template));
        ASSERT(renders_as(color_map, "{\"colors\":{"
                                          "\"yellow\":[255,255,0]"
                                          "}}"));

        // customize template for cyan and add a copy to the JSON structure
        jemi_string_set(color_name, "cyan");
        jemi_integer_set(red_val, 0);
        jemi_integer_set(grn_val, 255);
        jemi_integer_set(blu_val, 255);
        jemi_append_object(snippets, jemi_copy(snippet_template));
        ASSERT(renders_as(color_map, "{\"colors\":{"
                                     "\"yellow\":[255,255,0],"
                                     "\"cyan\":[0,255,255]"
                                     "}}"));

        // customize template for cyan and add a copy to the JSON structure
        jemi_string_set(color_name, "magenta");
        jemi_integer_set(red_val, 255);
        jemi_integer_set(grn_val, 0);
        jemi_integer_set(blu_val, 255);
        jemi_append_object(snippets, jemi_copy(snippet_template));
        ASSERT(renders_as(color_map, "{\"colors\":{"
                                     "\"yellow\":[255,255,0],"
                                     "\"cyan\":[0,255,255],"
                                     "\"magenta\":[255,0,255]"
                                     "}}"));

        // just for fun: https://encycolorpedia.com/693b58
        jemi_string_set(color_name, "aubergine");
        jemi_integer_set(red_val, 105);
        jemi_integer_set(grn_val, 59);
        jemi_integer_set(blu_val, 88);
        jemi_append_object(snippets, jemi_copy(snippet_template));
        ASSERT(renders_as(color_map, "{\"colors\":{"
                                     "\"yellow\":[255,255,0],"
                                     "\"cyan\":[0,255,255],"
                                     "\"magenta\":[255,0,255],"
                                     "\"aubergine\":[105,59,88]"
                                     "}}"));
    } while(false);

    printf("\nINFO: %ld out of %d free nodes available",
           jemi_available(),
           JEMI_POOL_SIZE);

    printf("\n... Finished test_jemi\n");
}

// *****************************************************************************
// Private (static) code

static void assert(bool expr, const char *str, const char *file, int line) {
  if (!expr) {
    printf("\nassertion %s failed at %s:%d", str, file, line);
  }
}

static void writer_fn(char c, void *arg) {
  json_writer_ctx *ctx = (json_writer_ctx *)arg;
  if (ctx->index < ctx->buflen) {
    ctx->buf[ctx->index++] = c;
  }
}

static bool renders_as(jemi_node_t *node, const char *expected) {
    json_writer_ctx ctx = {.buf=s_json_string,
                           .buflen=sizeof(s_json_string),
                           .index = 0};
    jemi_emit(node, writer_fn, &ctx);
    printf("\nrendering: %s", s_json_string);
    return strcmp(s_json_string, expected) == 0;
}

// *****************************************************************************
// End of file
