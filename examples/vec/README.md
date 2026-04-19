# Vec Pattern Examples

Examples of the `vec` pattern (dynamic array with pointer + length + capacity) from real C codebases.

## Valid

### `curl/appenddata.c`

- Extends pattern.

### `curl/dynbuf.c`

- Extends pattern.

### `curl/mem_nee.c`

- Extend Vec with a new buffer.
- Splits resizing buffer into a separate function.
- Extends pattern.

## Invalid

### `curl/URLPattern.c`

- Weird push pattern where we get a reference to the end of the array, then update the struct's values.
- Also requires a pre-condition that two slots are free before `glob_parse` is entered.

### `curl/termout.c`

- Uses some Windows API for push value.

```c
len = (DWORD)MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)rbuf, (int)rlen, global->term.buf, (int)len);
```

- Capacity also dynamically defined.

### `curl/readline.c`

- Stream pattern.

### `curl/mqttit.c`

- Stream pattern.

### `curl/rtspd_httprequest.c`

- Extend pattern.
- Missing capacity and capacity check.

### `curl/write_cb.c`

- Missing capacity check.
- Extend pattern.

### `curl/characterDataHandler.c`

- Missing capacity check.
- Extend pattern.

### `inih/missing_push_and_length_state.c`

- `inih_buffer_resize()` grows a line buffer but tracks **no length or push**. The function only manages `*max_line` (capacity) and reallocates; there is no field for the number of valid bytes written, and no push operation. Fails because a vec requires all three components.
