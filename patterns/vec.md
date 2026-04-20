# Vec

Represents a dynamic array that maps onto a Rust `Vec`.

## State

- **Pointer**: Points to contiguous heap memory.
- **Length**: Number of initialized elements.
- **Capacity**: Total allocated slots.

```c
typedef struct {
  T* data;
  size_t length;
  size_t capacity;
} Vec;
```

State may be split across separate parameters instead of a struct.

## Transitions

### Initialize

Allocate initial storage and zero-initialize metadata.

```c
data = malloc(initial_capacity * sizeof(T));
length = 0;
capacity = initial_capacity;
```

### Ensure Capacity

Guarantee `capacity >= required_size` before a push. Typically doubles allocation.

```c
if (length >= capacity) {
  size_t new_capacity = grow(capacity, required_size);
  T* new_data = realloc(data, new_capacity * sizeof(T));
  data = new_data;
  capacity = new_capacity;
}
```

### Push Value

Append a single element.

```c
data[length] = value;
length++;
```

### Extend

Append `n` elements from another buffer (bulk push via `memcpy`).

```c
memcpy(&data[length], src, n);
length += n;
```

### Custom Grow

Capacity growth factored into its own function, called by the caller before pushing.

## Variation Taxonomy

### Memory Layout

| Tag            | Description                                                 |
| -------------- | ----------------------------------------------------------- |
| `struct-based` | Pointer, length, and capacity in one struct                 |
| `split-args`   | Pointer, length, and capacity passed as separate parameters |

### Push Operation

| Tag           | Description                                  |
| ------------- | -------------------------------------------- |
| `push-single` | `data[length] = value; length++`             |
| `extend`      | `memcpy(&data[length], src, n); length += n` |

### Capacity Growth

| Tag           | Description                                    |
| ------------- | ---------------------------------------------- |
| `custom-Grow` | Growth logic factored into a separate function |

## Invalid Pattern Reasons

| Tag                  | Description                                                                                  |
| -------------------- | -------------------------------------------------------------------------------------------- |
| `missing-capacity`   | No capacity field; reallocs to exact `size + n` each call                                    |
| `missing-length`     | No length tracking; only manages allocation size                                             |
| `stream-push`        | Push reads from a file or socket (`fgets`, `read`, `sread`) instead of copying from a buffer |
| `platform-push`      | Push uses a platform API (`MultiByteToWideChar`) rather than `memcpy` or assignment          |
| `precondition-slots` | Validity depends on caller pre-validating free slot count before entering the push site      |
