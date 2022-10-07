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

#define JEMI_VERSION "1.1.0"

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

// List terminator value for jemi_obect()
#define jemi_list_end() ((jemi_node_t *)NULL)

// Signature for the jemi_emit function
typedef void (*jemi_writer_t)(char ch, void *arg);

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
 * @brief Create an JSON array with zero or more sub-elements.
 *
 * NOTE: jemi_array_end() must always be the last argument.  If you want to
 * create an array of zero elements (e.g. for subsequent calls to
 * `jemi_append_array()`), use the construct `jemi_array(jemi_array_end())`.
 */
jemi_node_t *jemi_array(jemi_node_t *element, ...);

/**
 * @brief Create a JSON object with zero or more key/value sub-elements.
 *
 * NOTE: jemi_object_end() must always be the last argument.  If you want to
 * create an object of zero elements (e.g. for subsequent calls to
 * `jemi_append_object()`), use the construct `jemi_object(jemi_object_end())`.
 */
jemi_node_t *jemi_object(jemi_node_t *element, ...);

/**
 * @brief Create a "disembodied list" of zero or more elements which are not
 * contained in an array nor in an object.  The result can be used subsequently
 * as an argument to jemi_append_array() or jemi_append_object().
 */
jemi_node_t *jemi_list(jemi_node_t *element, ...);

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
// duplicating a structure

/**
 * @brief Return a copy a JSON structure.
 */
jemi_node_t *jemi_copy(jemi_node_t *root);


// ******************************
// Composing and modifying JSON elements

/**
 * @brief Add one or more items to the array body.
 */
jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *items);

/**
 * @brief Add one or more items to the object body.
 */
jemi_node_t *jemi_append_object(jemi_node_t *object, jemi_node_t *items);

/**
 * @brief Add one or more items to a list.
 */
jemi_node_t *jemi_append_list(jemi_node_t *list, jemi_node_t *items);

/**
 * @brief Update contents of a JEMI_NUMBER node
 */
jemi_node_t *jemi_number_set(jemi_node_t *node, double number);

/**
 * @brief Update contents of a JEMI_STRING node
 *
 * NOTE: string must be null-terminated.
 */
jemi_node_t *jemi_string_set(jemi_node_t *node, const char *string);

/**
 * @brief Update contents of a JEMI_BOOL node
 */
jemi_node_t *jemi_bool_set(jemi_node_t *node, bool boolean);

// ******************************
// Outputting JSON strings

/**
 * @brief Output a JEMI structure.
 *
 * @param root the root of the JEMI structure.
 * @param writer writer function wich char and user context args
 * @param user-supplied context
 */
void jemi_emit(jemi_node_t *root, jemi_writer_t writer_fn, void *arg);

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
