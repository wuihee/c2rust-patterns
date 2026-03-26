# parson: dynamic_array_append evidence

This note ties the minimized extraction to locations in `examples/parson/parson.c`.

## Source locations

- `examples/parson/parson.c:771` (`json_array_add`)
  - checks growth condition: `array->count >= array->capacity`
  - computes growth: `MAX(array->capacity * 2, STARTING_CAPACITY)`
  - delegates resize and propagates failure
  - appends value at `array->items[array->count]`
  - increments logical length via `array->count++`

- `examples/parson/parson.c:784` (`json_array_resize`)
  - allocates new storage for item pointers
  - copies prior elements with `memcpy`
  - frees old storage
  - updates `array->items` and `array->capacity`

## Notes

The semantic append operation is split across two helpers and uses allocate-copy-free resize rather than `realloc`. Pattern matching should treat this as the same high-level operation as typical vector push.
