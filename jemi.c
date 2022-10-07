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
static void emit_aux(jemi_node_t *root, jemi_writer_t writer_fn, void *arg, bool is_obj);

/**
 * @brief Make a copy of a node and its contents, including children nodes,
 * but not siblings.
 */
static jemi_node_t *copy_node(jemi_node_t *node);

/**
 * @brief Write a string to the writer_fn, a byte at a time.
 */
static void emit_string(jemi_writer_t writer_fn, void *arg, const char *buf);

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

jemi_node_t *jemi_list(jemi_node_t *element, ...) {
    va_list ap;
    jemi_node_t *first = element;

    va_start(ap, element);
    while(element != NULL) {
        element->sibling = va_arg(ap, jemi_node_t *);
        element = element->sibling;
    }
    return first;
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

jemi_node_t *jemi_copy(jemi_node_t *root) {
    jemi_node_t *r2 = NULL;
    jemi_node_t *prev = NULL;
    jemi_node_t *node;

    while ((node = copy_node(root)) != NULL) {
        if (r2 == NULL) {
            // first time through the loop: save pointer to first element
            r2 = node;
        }
        if (prev != NULL) {
            prev->sibling = node;
        }
        prev = node;
        root = root->sibling;
    }
    return r2;
}

jemi_node_t *jemi_append_array(jemi_node_t *array, jemi_node_t *items) {
    if (array) {
        array->children = jemi_append_list(array->children, items);
    }
    return array;
}

jemi_node_t *jemi_append_object(jemi_node_t *object, jemi_node_t *items) {
    if (object) {
        object->children = jemi_append_list(object->children, items);
    }
    return object;
}

jemi_node_t *jemi_append_list(jemi_node_t *list, jemi_node_t *items) {
    if (list == NULL) {
        return items;
    } else {
        jemi_node_t *prev = NULL;
        jemi_node_t *node = list;
        // walk list to find last element
        while (node) {
            prev = node;
            node = node->sibling;
        }
        // prev is now null or points to last sibling in list.
        if (prev) {
            prev->sibling = items;
        }
      return list;
    }
}

jemi_node_t *jemi_number_set(jemi_node_t *node, double number) {
    if (node) {
        node->number = number;
    }
    return node;
}

/**
 * @brief Update contents of a JEMI_STRING node
 *
 * NOTE: string must be null-terminated.
 */
jemi_node_t *jemi_string_set(jemi_node_t *node, const char *string) {
    if (node) {
        node->string = string;
    }
    return node;
}

/**
 * @brief Update contents of a JEMI_BOOL node
 */
jemi_node_t *jemi_bool_set(jemi_node_t *node, bool boolean) {
    if (node) {
        node->type = boolean ? JEMI_TRUE : JEMI_FALSE;
    }
    return node;
}


void jemi_emit(jemi_node_t *root, jemi_writer_t writer_fn, void *arg) {
  emit_aux(root, writer_fn, arg, false);
  writer_fn('\0', arg);
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

static void emit_aux(jemi_node_t *root, jemi_writer_t writer_fn, void *arg, bool is_obj) {
  int count = 0;
  jemi_node_t *node = root;
  while (node) {
    if (is_obj && (count & 1)) {
      writer_fn(':', arg);
    } else if (count > 0) {
      writer_fn(',', arg);
    }
    switch (node->type) {
    case JEMI_OBJECT: {
      writer_fn('{', arg);
      emit_aux(node->children, writer_fn, arg, true);
      writer_fn('}', arg);
    } break;

    case JEMI_ARRAY: {
      writer_fn('[', arg);
      emit_aux(node->children, writer_fn, arg, false);
      writer_fn(']', arg);
    } break;

    case JEMI_NUMBER: {
      char buf[20];
      snprintf(buf, sizeof(buf), "%f", node->number);
      emit_string(writer_fn, arg, buf);
    } break;

    case JEMI_STRING: {
      writer_fn('"', arg);
      emit_string(writer_fn, arg, node->string);
      writer_fn('"', arg);
    } break;

    case JEMI_TRUE: {
      emit_string(writer_fn, arg, "true");
    } break;

    case JEMI_FALSE: {
      emit_string(writer_fn, arg, "false");
    } break;

    case JEMI_NULL: {
      emit_string(writer_fn, arg, "null");
    } break;
    }
    count += 1;
    node = node->sibling;
  }
}

static jemi_node_t *copy_node(jemi_node_t *node) {
    jemi_node_t *copy;
    if (node == NULL) {
        copy = NULL;
    } else {
        copy = jemi_alloc(node->type);
        switch(node->type) {
            case JEMI_ARRAY:
            case JEMI_OBJECT: {
                copy->children = jemi_copy(node->children);
            } break;
            case JEMI_STRING: {
                copy->string = node->string;
            } break;
            case JEMI_NUMBER: {
                copy->number = node->number;
            }
            default: {
                // no action needed
            }
        }  // switch()
    }
    return copy;
}

static void emit_string(jemi_writer_t writer_fn, void *arg, const char *buf) {
  while (*buf) {
    writer_fn(*buf++, arg);
  }
}

// *****************************************************************************
// End of file
