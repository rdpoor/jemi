/**
 * @file jemi.h
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
 */

/**
 * @brief a pure-C JEMI serializer for embedded systems
 */

#ifndef _JEMI_H_
#define _JEMI_H_

// *****************************************************************************
// Includes

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

// *****************************************************************************
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

typedef enum {
  JEMI_OBJECT,
  JEMI_ARRAY,
  JEMI_NUMBER,
  JEMI_STRING,
  JEMI_TRUE,
  JEMI_FALSE,
  JEMI_NULL
} jemi_type_t;

typedef struct _jemi_node {
  struct _jemi_node *sibling; // any object may have siblings...
  jemi_type_t type;
  union {
    struct _jemi_node *children; // for JEMI_ARRAY or JEMI_OBJECT
    double number;               // for JEMI_NUMBER
    const char *string;          // for JEMI_STRING
  };
} jemi_node_t;

// List terminator value for jemi_array()
#define jemi_array_end() ((jemi_node_t *)NULL)

// List terminator value for jemi_obect()
#define jemi_object_end() ((jemi_node_t *)NULL)

// Signature for the jemi_emit function
typedef void (*jemi_writer_t)(char ch);

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the jemi system.
 *
 * Example:
 *
 *     #include "jemi.h"
 *
 *     #define JEMI_POOL_SIZE 30  // maximum number of nodes we expect to create
 *     static jemi_node_t jemi_pool[JEMI_POOL_SIZE];
 *
 *     jemi_init(jemi_pool, JEMI_POOL_SIZE)
 *
 * @param pool User-supplied vector of jemi_node objects.
 * @param pool_size Number of jemi_node objects in the pool.
 */
void jemi_init(jemi_node_t *pool, size_t pool_size);

/**
 * @brief Release all jemi_node objects back to the pool.
 */
void jemi_reset(void);

// ******************************
// Creating JSON elements

/**
 * @brief Create an array of elements.
 *
 * NOTE: jemi_array_end() must always be the last argument.  If you want to
 * create an array of zero elements (e.g. for subsequent calls to
 * `jemi_append_array()`), use the construct `jemi_array(jemi_array_end())`.
 */
jemi_node_t *jemi_array(jemi_node_t *element, ...);

/**
 * @brief Create a JSON object of key/value pairs.
 *
 * NOTE: jemi_object_end() must always be the last argument.  If you want to
 * create an object of zero elements (e.g. for subsequent calls to
 * `jemi_append_object()`), use the construct `jemi_object(jemi_object_end())`.
 */
jemi_node_t *jemi_object(jemi_node_t *element, ...);

/**
 * @brief Create a JSON number.
 */
jemi_node_t *jemi_number(double value);

/**
 * @brief Create a JSON string.
 *
 * NOTE: string must be null-terminated.
 */
jemi_node_t *jemi_string(const char *string);

/**
 * @brief Create a JSON boolean (true or false).
 */
jemi_node_t *jemi_bool(bool boolean);

/**
 * @brief Create a JSON true object.
 */
jemi_node_t *jemi_true(void);

/**
 * @brief Create a JSON false object.
 */
jemi_node_t *jemi_false(void);

/**
 * @brief Create a JSON null object.
 */
jemi_node_t *jemi_null(void);

// ******************************
// Composing JSON elements

/**
 * @brief Add an element to an array.
 */
jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *element);

/**
 * @brief Add a key / value pair to an object.
 *
 * NOTE: the key object a null-terminated C string, not a jemi_string().
 */
jemi_node_t *jemi_append_object(jemi_node_t *object, const char *key,
                                jemi_node_t *element);

// ******************************
// Outputting JSON strings

/**
 * @brief Output a JEMI structure.
 *
 * @param root the root of the JEMI structure.
 * @param writer a function that accepts a single character to be emitted.
 */
void jemi_emit(jemi_node_t *root, jemi_writer_t writer_fn);

/**
 * @brief Return the number of available jemi_node objects.
 *
 * Note: jemi does its best to "silently fail" without causing a bus error.
 * This function can tell you if you've run out of available jemi_node objects.
 */
size_t jemi_available(void);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _JEMI_H_ */
