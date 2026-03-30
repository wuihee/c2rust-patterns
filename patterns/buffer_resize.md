# Pattern: `buffer_resize`

Ensure a growable contiguous buffer has enough spare capacity for an upcoming write, without changing logical length.

## Observed In

- [`parson`](../../examples/parson/buffer_resize.c)
- [`inih`](../../examples/inih/buffer_resize.c)

## Canonical Code Shape

```c
int __buffer_resize(
    void **data,
    size_t *cap,
    size_t elem_size,
    size_t min_needed
) {
    size_t capacity = *cap;

    // 1. Capacity Guard
    if (min_needed <= capacity) {
        return 0;
    }

    // 2. Growth Policy
    size_t new_capacity = capacity;
    while (new_capacity < min_needed) {
        new_capacity = grow(new_capacity, min_needed);
    }

    // 3. Reallocation
    void *new_buffer = realloc(*data, new_capacity * elem_size);

    // 4. Error Handling (Optional?)
    if (new_buffer == NULL) {
        return -1;
    }

    // 5. Commit
    *data = new_buffer;
    *cap = new_capacity;

    return 0;
}
```
