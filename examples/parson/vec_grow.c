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
