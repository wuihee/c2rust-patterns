// Demo extraction for the dynamic_array_append pattern.
// Replace with real snippets from target repositories.

#include <stdlib.h>
#include <string.h>

typedef struct {
  int *data;
  size_t len;
  size_t cap;
} IntVec;

int int_vec_push(IntVec *v, int value) {
  if (v->len == v->cap) {
    size_t new_cap = (v->cap == 0) ? 8 : v->cap * 2;
    int *new_data = (int *)realloc(v->data, new_cap * sizeof(int));
    if (!new_data)
      return -1;
    v->data = new_data;
    v->cap = new_cap;
  }

  memcpy(&v->data[v->len], &value, sizeof(int));
  v->len += 1;
  return 0;
}
