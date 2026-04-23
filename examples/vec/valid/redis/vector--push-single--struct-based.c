// repos/src/vector.h
// repos/src/vector.c

#include <stddef.h>
#include <string.h>

#define unlikely(x) __builtin_expect(!!(x), 0)

#define VEC_DEFAULT_INITCAP 8

__attribute__((malloc, alloc_size(1), noinline)) void *zmalloc(size_t size);
__attribute__((alloc_size(2), noinline)) void *zrealloc(void *ptr, size_t size);

typedef struct vec {
  size_t size;  /* [Length] Number of elements in the vector. */
  size_t cap;   /* [Capacity] Capacity of the vector. */
  void **data;  /* [Vec Pointer] Heap-allocated storage or refers to stack. */
  void **stack; /* Optional stack buffer. */
} vec;

/* Ensure capacity is at least mincap. */
void vecReserve(vec *v, size_t mincap) {
  void **newdata;

  if (mincap <= v->cap)
    return;

  /* If no heap storage is used yet, allocate and copy from stack if needed. */
  if (v->data == v->stack) {
    newdata = zmalloc(mincap * sizeof(void *));
    if (v->size)
      memcpy(newdata, v->data, v->size * sizeof(void *));
  } else {
    newdata = zrealloc(v->data, mincap * sizeof(void *));
  }

  v->data = newdata;
  v->cap = mincap;
}

void vecPush(vec *v, void *value) {
  // [Ensure Capacity]
  if (unlikely(v->size == v->cap)) {
    size_t newcap = (v->cap > 0) ? v->cap * 2 : VEC_DEFAULT_INITCAP;
    vecReserve(v, newcap);
  }

  // [Push Value]
  v->data[v->size++] = value;
}
