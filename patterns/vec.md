# Vec

Represents a dynamic array that maps onto a Rust `Vec`.

## State

- Data Pointer: A pointer to contiguous memory.
- Length: Number of initialized elements.
- Capacity: Total allocated space.

```c
typedef struct {
  T* data;
  size_t length;
  size_t capacity;
} Vec;
```

## Invariants

- Capacity: `length <= capacity`

## Semantic Patterns

### Push

Append a single element to the end of the array.

- **Capacity Guard**: Check that there is sufficient space in the vec.
- **Push Value**: Append the new value to the vec.

```c
// Capacity Guard
if (length >= capacity) {
  resize();
}

// Push Value
data[length] = value;

// Length Update
length++;
```

## Capability Patterns

### Resize

Resize the array to ensure sufficient capacity for future elements.

- **Capacity Growth**: `new_capacity` > `old_capacity`
- **Reallocation**
  - `realloc`
  - `malloc` + `memcpy` + `free`
  - Custom Allocator

```c
// Capacity Growth
size_t new_capacity = grow(capacity);

// Reallocation
T* new_data = new_memory(data, new_capacity);

// Data Update
data = new_data;

// Capacity Update
capacity = new_capacity;
```
