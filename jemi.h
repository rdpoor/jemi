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
  jemi_type_t type;
  // NB: a tail pointer would make adding elements more efficient, but we
  // choose small size over speed.
  struct _jemi_node *link;        // for JEMI_OBJECT or JEMI_ARRAY
  union {
    double number;                // for JEMI_NUMBER
    const char *string;           // for JEMI_STRING
  };
} jemi_node_t;

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

/**
 * @brief Allocate and initialize a jemi_node from the pool.
 *
 * Note: there is no corresponding jemi_free(), because the expected usage model
 * is that you'll build up your JEMI structure before calling `jemi_emit()`.
 * After that, you can call jemi_init() to reinitialize everything.
 *
 * @return A jemi_node or NULL if no more nodes are avaialble.
 */
jemi_node_t *jemi_alloc_object(void);
jemi_node_t *jemi_alloc_array(void);
jemi_node_t *jemi_alloc_number(double value);
jemi_node_t *jemi_alloc_string(const char *string);
jemi_node_t *jemi_alloc_bool(bool boolean);
jemi_node_t *jemi_alloc_true(void);
jemi_node_t *jemi_alloc_false(void);
jemi_node_t *jemi_alloc_null(void);

/**
 * @brief Add a key / element pair to an object.
 */
jemi_node_t *jemi_append_object(jemi_node_t *object, const char *key, jemi_node_t *element);

/**
 * @brief Add an element to an array.
 */
jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *element);

/**
 * @brief Append a series of elements to a collection (object or array)
 */
jemi_node_t *jemi_append(jemi_node_t *collection, ...);

/**
 * @brief Emit the JEMI structure.
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
