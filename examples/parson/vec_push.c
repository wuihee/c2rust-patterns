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

JSON_Status json_array_resize(JSON_Array *array, size_t new_capacity);

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
