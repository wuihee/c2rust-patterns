// Extracted/minimized from parson's JSON array growth path.

#include <stdlib.h>
#include <string.h>

typedef enum {
  JSONFailure = -1,
  JSONSuccess = 0,
} JSON_Status;

typedef struct JSON_Value JSON_Value;

typedef struct {
  JSON_Value **items;
  size_t count;
  size_t capacity;
} JSON_Array;

#define STARTING_CAPACITY 16
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static JSON_Status json_array_resize(JSON_Array *array, size_t new_capacity) {
  JSON_Value **new_items = NULL;
  if (new_capacity == 0) {
    return JSONFailure;
  }

  new_items = (JSON_Value **)malloc(new_capacity * sizeof(JSON_Value *));
  if (new_items == NULL) {
    return JSONFailure;
  }

  if (array->items != NULL && array->count > 0) {
    memcpy(new_items, array->items, array->count * sizeof(JSON_Value *));
  }

  free(array->items);
  array->items = new_items;
  array->capacity = new_capacity;
  return JSONSuccess;
}

static JSON_Status json_array_add(JSON_Array *array, JSON_Value *value) {
  if (array->count >= array->capacity) {
    size_t new_capacity = MAX(array->capacity * 2, STARTING_CAPACITY);
    if (json_array_resize(array, new_capacity) != JSONSuccess) {
      return JSONFailure;
    }
  }

  array->items[array->count] = value;
  array->count++;
  return JSONSuccess;
}
