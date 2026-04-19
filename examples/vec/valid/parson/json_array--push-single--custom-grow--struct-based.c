// repos/parson/parson.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_CAPACITY 16

typedef enum {
  JSONFailure = -1,
  JSONSuccess = 0,
} JSON_Status;

typedef struct JSON_Value JSON_Value;

typedef struct {
  // [Pointer]
  JSON_Value **items;

  // [Length]
  size_t count;

  // [Capacity]
  size_t capacity;
} JSON_Array;

// [Custom Grow] — allocates a fresh block and copies existing items over.
static JSON_Status json_array_resize(JSON_Array *array, size_t new_capacity) {
  if (new_capacity == 0)
    return JSONFailure;

  JSON_Value **new_items = malloc(new_capacity * sizeof(JSON_Value *));
  if (!new_items)
    return JSONFailure;

  if (array->items && array->count > 0)
    memcpy(new_items, array->items, array->count * sizeof(JSON_Value *));

  free(array->items);
  array->items = new_items;
  array->capacity = new_capacity;
  return JSONSuccess;
}

static JSON_Status json_array_add(JSON_Array *array, JSON_Value *value) {
  // [Ensure Capacity: Custom Grow]
  if (array->count >= array->capacity) {
    size_t new_capacity = array->capacity * 2;
    if (new_capacity < STARTING_CAPACITY)
      new_capacity = STARTING_CAPACITY;
    if (json_array_resize(array, new_capacity) != JSONSuccess)
      return JSONFailure;
  }

  // [Push Value: Single]
  array->items[array->count] = value;
  array->count++;
  return JSONSuccess;
}
