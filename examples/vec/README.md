# Vec Pattern Examples

Examples of the `vec` pattern (dynamic array with pointer + length + capacity) from real C codebases.

File names encode variation tags: `{function}--{tags}.c`

---

## Valid

### `curl/appenddata--extend--split-args.c`

- Memory Layout: Split-Args
- Push Operation: Extend

### `curl/dynbuf--extend--struct-based.c`

- Memory Layout: Struct-Based
- Push Operation: Extend

### `curl/mem_need--extend--custom-grow--struct-based.c`

- Memory Layout: Struct-Based
- Push Operation: Extend
- Capacity Growth: Custom-Grow

### `curl/h2_stream_ctx--push-single--struct-based.c`

- Memory Layout: Struct-Based
- Push Operation: Push-Single
- Initializes dyamic array with `malloc` if it doesn't exist yet.

### `parson/json_array--push-single--custom-grow--struct-based.c`

- Memory-Layout: Struct-Based
- Push Operation: Push-Single
- Capacity Growth: Custom-Grow

---

## Invalid

### `curl/write_cb--extend--missing-capacity.c`

- Invalid Reason: Missing Capacity
- Push Operation: Extend

### `curl/characterDataHandler--extend--missing-capacity.c`

- Invalid Reason: Missing Capacity
- Push Operation: Extend

### `curl/readline--stream-push.c`

- Invalid Reason: Stream Push (`fgets`)

### `curl/mqttit--stream-push.c`

- Invalid Reason: Stream Push (`sread`)

### `curl/termout--platform-push.c`

- Invalid Reason: Platform Push
- Push is `MultiByteToWideChar(CP_UTF8, ..., global->term.buf, ...)`, a Windows API call that writes converted UTF-16 directly into the buffer. No standard `memcpy` or assignment.

### `curl/URLPattern--precondition-slots.c`

- Invalid Reason: Precondition Slots
- `glob_fixed` and `glob_set` write into `pattern[pnum]` directly without checking capacity; they rely on `add_glob` (called by the parent `glob_parse`) to handle the capacity check and length increment after the fact. Push validity depends on caller-enforced slot pre-allocation.

### `curl/rtspd_httprequest--extend--missing-capacity.c`

- Invalid Reason: Missing Capacity
- Push Operation: Extend

### `inih/inih_buffer_resize--stream-push.c`

- Invalid Reason: Stream Push (`reader`)
