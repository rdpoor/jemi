/**
 * @file jemi.c
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

// *****************************************************************************
// Includes

#include "jemi.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static) storage

jemi_node_t *s_jemi_pool;          // user supplied block of nodes
size_t s_jemi_pool_size;           // number of user-supplied nodes
jemi_node_t *s_jemi_freelist;      // next available node (or null if empty)

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Pop one element from the freelist.  If available, set the type and
 * return it, else return NULL.
 */
static jemi_node_t *jemi_alloc(jemi_type_t type);

/**
 * @brief Write a string to the writer_fn, a byte at a time.
 */
static void emit_string(jemi_writer_t writer_fn, const char *buf);


// *****************************************************************************
// Public code

void jemi_init(jemi_node_t *pool, size_t pool_size) {
  s_jemi_pool = pool;
  s_jemi_pool_size = pool_size;
  jemi_reset();
}

void jemi_reset(void) {
  memset(s_jemi_pool, 0, s_jemi_pool_size * sizeof(jemi_node_t));
  // rebuild the freelist, using node->link as the link field
  jemi_node_t *next = NULL;  // end of the linked list
  for (int i=0; i<s_jemi_pool_size; i++) {
    jemi_node_t *node = &s_jemi_pool[i];
    node->link = next;
    next = node;
  }
  s_jemi_freelist = next;  // reset head of the freelist
}

jemi_node_t *jemi_alloc_object(void) { return jemi_alloc(JEMI_OBJECT); }

jemi_node_t *jemi_alloc_array(void) { return jemi_alloc(JEMI_ARRAY); }

jemi_node_t *jemi_alloc_number(double value) {
  jemi_node_t *node = jemi_alloc(JEMI_NUMBER);
  if (node) {
    node->number = value;
  }
  return node;
}

jemi_node_t *jemi_alloc_string(const char *string) {
  jemi_node_t *node = jemi_alloc(JEMI_STRING);
  if (node) {
    node->string = string;
  }
  return node;
}

jemi_node_t *jemi_alloc_bool(bool boolean) {
  if (boolean) {
    return jemi_alloc_true();
  } else {
    return jemi_alloc_false();
  }
}

jemi_node_t *jemi_alloc_true(void) { return jemi_alloc(JEMI_TRUE); }

jemi_node_t *jemi_alloc_false(void) { return jemi_alloc(JEMI_FALSE); }

jemi_node_t *jemi_alloc_null(void)  { return jemi_alloc(JEMI_NULL); }

jemi_node_t *jemi_append_object(jemi_node_t *object, const char *key, jemi_node_t *element) {
  if (object && element) {
    jemi_node_t *keynode = jemi_alloc_string(key);
    // need all three nodes for this to work...
    if (keynode) {
      // get pointer to last child in the object's list...
      jemi_node_t *last = object;
      while (last->link) {
        last = last->link;
      }
      // last is the last in the list, last->next is known to be NULL.
      // append the key and the element.
      last->link = keynode;
      keynode->link = element;
    }
  }
  return object;
}

jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *element) {
  if (array && element) {
    // need both for this to work...
    // get pointer to the last child in the array's list...
    jemi_node_t *last = array;
    while (last->link) {
      last = last->link;
    }
    // last is the last in the list, last->next is known to be NULL.
    // Append the element.
    last->link = element;
  }
  return array;
}

jemi_node_t *jemi_append(jemi_node_t *collection, ...) {
  va_list ap;
  jemi_node_t *node = collection;
  va_start(ap, collection);
  while((node->link = va_arg(ap, jemi_node_t *)) != NULL) {
    node = node->link;
  }
  return collection;
}

void jemi_emit(jemi_node_t *root, jemi_writer_t writer_fn) {
  int count = 0;
  jemi_node_t *node = root;
  while(node) {
    if (root->type == JEMI_ARRAY) {
      // Arrays: count = 0 is the array node, count = 1 is the first element,
      // count = 2 is the second, ...Type a comma before all elements except
      // for the first.
      if (count > 2) {
        writer_fn(',');
      }
    } else if (root->type == JEMI_OBJECT) {
      // Arrays: count = 0 is the array node, count = 1 is the first element,
      // count = 2 is the second, etc.  Type a colon before the second element
      // and all subsequent even element, type a comma before the third element
      // and all seubsequent odd elements.
      if (count >= 2 && ((count & 1) == 0)) {
        writer_fn(':');
      } else if (count >= 3) {
        writer_fn(',');
      }
    }
    switch(node->type) {
      case JEMI_OBJECT: {
        writer_fn('{');
        jemi_emit(node->link, writer_fn);
        writer_fn('}');
      } break;

      case JEMI_ARRAY: {
        writer_fn('[');
        jemi_emit(node->link, writer_fn);
        writer_fn(']');
      } break;

      case JEMI_NUMBER: {
        char buf[20];
        snprintf(buf, sizeof(buf), "%f", node->number);
        emit_string(writer_fn, buf);
      } break;

      case JEMI_STRING: {
        writer_fn('"');
        emit_string(writer_fn, node->string);
        writer_fn('"');
      } break;

      case JEMI_TRUE: {
        emit_string(writer_fn, "true");
      } break;

      case JEMI_FALSE: {
        emit_string(writer_fn, "false");
      } break;

      case JEMI_NULL: {
        emit_string(writer_fn, "null");
      } break;
    }
    count += 1;
    node = node->link;
  }
}

size_t jemi_available(void) {
  size_t count = 0;

  jemi_node_t *node = s_jemi_freelist;
  while (node) {
    count += 1;
    node = node->link;
  }
  return count;
}

// *****************************************************************************
// Private (static) code

static jemi_node_t *jemi_alloc(jemi_type_t type) {
  // pop one node from the freelist
  jemi_node_t *node = s_jemi_freelist;
  if (node) {
    s_jemi_freelist = node->link;
    node->link = NULL;
    node->type = type;
  }
  return node;
}

static void emit_string(jemi_writer_t writer_fn, const char *buf) {
  while (*buf) {
    writer_fn(*buf++);
  }
}

// *****************************************************************************
// End of file
