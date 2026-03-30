#include <stdlib.h>
#include <string.h>

#define STARTING_CAPACITY 16
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

static JSON_Status json_array_resize(JSON_Array *array, size_t new_capacity) {
  JSON_Value **new_items = NULL;
  if (new_capacity == 0) {
    return JSONFailure;
  }

  // Reallocation
  new_items = (JSON_Value **)malloc(new_capacity * sizeof(JSON_Value *));

  // Error Handling
  if (new_items == NULL) {
    return JSONFailure;
  }

  // Reallocation
  if (array->items != NULL && array->count > 0) {
    memcpy(new_items, array->items, array->count * sizeof(JSON_Value *));
  }

  // Commit
  free(array->items);
  array->items = new_items;
  array->capacity = new_capacity;

  return JSONSuccess;
}

static JSON_Status json_array_add(JSON_Array *array, JSON_Value *value) {
  // Capacity Guard
  if (array->count >= array->capacity) {
    // Growth Policy
    size_t new_capacity = MAX(array->capacity * 2, STARTING_CAPACITY);

    if (json_array_resize(array, new_capacity) != JSONSuccess) {
      return JSONFailure;
    }
  }

  array->items[array->count] = value;
  array->count++;
  return JSONSuccess;
}
