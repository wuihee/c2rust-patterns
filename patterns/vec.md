# Vec

Represents a dynamic array that maps onto a Rust `Vec`.

## State

- **Vec Pointer**: A pointer to contiguous memory.
- **Length**: Number of initialized elements.
- **Capacity**: Total allocated space.

```c
typedef struct {
  T* vec;
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

## TODO

- repos/curl/src/tool_urlglob.c:394, repos/curl/src/tool_urlglob.c:396
- repos/curl/lib/http2.c:1481, repos/curl/lib/http2.c:1482
- repos/curl/lib/getenv.c:42
- repos/curl/src/tool_cb_wrt.c:216
- test/example grow-buffers:  
  repos/curl/tests/server/getpart.c:108, repos/curl/tests/server/getpart.c:113, repos/curl/tests/server/getpart.c:170, repos/curl/tests/server/mqttd.c:474, repos/curl/tests/server/rtspd.c:329, repos/curl/docs/examples/getinmemory.c:45, repos/curl/docs/examples/postinmemory.c:44, repos/curl/docs/examples/http2-pushinmemory.c:43, repos/curl/docs/examples/xmlstream.c:76, repos/curl/docs/examples/log_failed_transfers.c:101, repos/curl/docs/examples/crawler.c:69
