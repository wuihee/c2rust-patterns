# Vec

Represents a dynamic array that maps onto a Rust `Vec`.

## State

- **Data Pointer**: A pointer to contiguous memory.
- **Length**: Number of initialized elements.
- **Capacity**: Total allocated space.

```c
typedef struct {
  T* data;
  size_t length;
  size_t capacity;
} Vec;
```

## Transitions

### Push Value

Append `n` elements to the end of the array.

```c
// Append One
data[length] = value;
length++;

// Append Many
memcpy(&data[length], src, n);
length += n;
```

### Ensure Capcity

Ensure `capacity` is sufficient for the required size.

- **Post Condition**: `capacity >= required_size`

```c
if (length >= capacity) {
  size_t new_capacity = grow(capacity, requird_size);
  T* new_data = new_memory(data, new_capacity);
  data = new_data;
  capacity = new_capacity;
}
```
