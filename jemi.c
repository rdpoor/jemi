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

jemi_node_t *s_jemi_pool;     // user supplied block of nodes
size_t s_jemi_pool_size;      // number of user-supplied nodes
jemi_node_t *s_jemi_freelist; // next available node (or null if empty)

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Pop one element from the freelist.  If available, set the type and
 * return it, else return NULL.
 */
static jemi_node_t *jemi_alloc(jemi_type_t type);

/**
 * @brief Print a node or a list of nodes.
 */
static void emit_aux(jemi_node_t *root, jemi_writer_t writer_fn, bool is_obj);

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
  // rebuild the freelist, using node->sibling as the link field
  jemi_node_t *next = NULL; // end of the linked list
  for (int i = 0; i < s_jemi_pool_size; i++) {
    jemi_node_t *node = &s_jemi_pool[i];
    node->sibling = next;
    next = node;
  }
  s_jemi_freelist = next; // reset head of the freelist
}

jemi_node_t *jemi_array(jemi_node_t *element, ...) {
  va_list ap;
  jemi_node_t *root = jemi_alloc(JEMI_ARRAY);

  va_start(ap, element);
  root->children = element;
  while(element != NULL) {
      element->sibling = va_arg(ap, jemi_node_t *);
      element = element->sibling;
  }
  return root;
}

jemi_node_t *jemi_object(jemi_node_t *element, ...) {
    va_list ap;
    jemi_node_t *root = jemi_alloc(JEMI_OBJECT);

    va_start(ap, element);
    root->children = element;
    while(element != NULL) {
        element->sibling = va_arg(ap, jemi_node_t *);
        element = element->sibling;
    }
    return root;
  }

jemi_node_t *jemi_number(double value) {
  jemi_node_t *node = jemi_alloc(JEMI_NUMBER);
  if (node) {
    node->number = value;
  }
  return node;
}

jemi_node_t *jemi_string(const char *string) {
  jemi_node_t *node = jemi_alloc(JEMI_STRING);
  if (node) {
    node->string = string;
  }
  return node;
}

jemi_node_t *jemi_bool(bool boolean) {
  if (boolean) {
    return jemi_true();
  } else {
    return jemi_false();
  }
}

jemi_node_t *jemi_true(void) { return jemi_alloc(JEMI_TRUE); }

jemi_node_t *jemi_false(void) { return jemi_alloc(JEMI_FALSE); }

jemi_node_t *jemi_null(void) { return jemi_alloc(JEMI_NULL); }

jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *element) {
  // Need both an array and an element for this to work sensibly.
  if ((array == NULL) || (element == NULL)) {
    return array;
  }
  if (array->children == NULL) {
    array->children = element;
  } else {
    // walk list to find last element
    jemi_node_t *last = array->children;
    while (last->sibling) {
      last = last->sibling;
    }
    // Append the element after the last element
    last->sibling = element;
  }
  return array;
}

jemi_node_t *jemi_append_object(jemi_node_t *object, const char *key,
                                jemi_node_t *value) {
  // Need an object, a key and a value for this to work sensibly.
  if ((object == NULL) || (value == NULL)) {
      return object;
  }
  jemi_node_t *keynode = jemi_string(key);
  if (keynode == NULL) {
      return object;
  }
  keynode->sibling = value;

  if (object->children == NULL) {
    object->children = keynode;
  } else {
    // walk list to find last element
    jemi_node_t *last = object->children;
    while (last->sibling) {
      last = last->sibling;
    }
    // Append the key and the value elements after the last element
    last->sibling = keynode;
  }
  return object;
}

void jemi_emit(jemi_node_t *root, jemi_writer_t writer_fn) {
  emit_aux(root, writer_fn, false);
}

size_t jemi_available(void) {
  size_t count = 0;

  jemi_node_t *node = s_jemi_freelist;
  while (node) {
    count += 1;
    node = node->sibling;
  }
  return count;
}

// *****************************************************************************
// Private (static) code

static jemi_node_t *jemi_alloc(jemi_type_t type) {
  // pop one node from the freelist
  jemi_node_t *node = s_jemi_freelist;
  if (node) {
    s_jemi_freelist = node->sibling;
    node->sibling = NULL;
    node->type = type;
  }
  return node;
}

static void emit_aux(jemi_node_t *root, jemi_writer_t writer_fn, bool is_obj) {
  int count = 0;
  jemi_node_t *node = root;
  while (node) {
    if (is_obj && (count & 1)) {
      writer_fn(':');
    } else if (count > 0) {
      writer_fn(',');
    }
    switch (node->type) {
    case JEMI_OBJECT: {
      writer_fn('{');
      emit_aux(node->children, writer_fn, true);
      writer_fn('}');
    } break;

    case JEMI_ARRAY: {
      writer_fn('[');
      emit_aux(node->children, writer_fn, false);
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
    node = node->sibling;
  }
}

static void emit_string(jemi_writer_t writer_fn, const char *buf) {
  while (*buf) {
    writer_fn(*buf++);
  }
}

// *****************************************************************************
// End of file
