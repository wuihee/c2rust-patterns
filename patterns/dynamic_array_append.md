# Pattern: Dynamic Array Append

## Name

`dynamic_array_append`

## Semantic Intent

Append one element to a growable contiguous buffer, expanding capacity when full while preserving existing elements.

## Observed In (projects/files)

- Generic C utility libraries: custom vector/list implementations in `src/*.{c,h}`
- Parser/compiler frontends: token or AST node buffers
- Networking/system tools: request/response accumulator arrays
- `examples/parson/parson.c`:
  - `json_array_add` grows when `count >= capacity`, writes at `items[count]`, then increments `count`
  - `json_array_resize` allocates new storage, `memcpy`s old elements, frees old storage, and updates `capacity`

Concrete examples for this repository are tracked under:

- `examples/demo_project/dynamic_array_append.c`
- `examples/parson/dynamic_array_append.c`

## Canonical Code Shape

```c
int __vec_append(void **data, size_t *len, size_t *cap, size_t elem_size, const void *elem) {
  if (*len == *cap) {
    size_t new_cap = (*cap == 0) ? 8 : (*cap * 2);
    if (new_cap < *cap) return -1; // overflow guard

    void *new_data = realloc(*data, new_cap * elem_size);
    if (!new_data) return -1;

    *data = new_data;
    *cap = new_cap;
  }

  memcpy((char *)(*data) + (*len * elem_size), elem, elem_size);
  *len += 1;
  return 0;
}
```

Candidate normalized call site form:

```c
if (__vec_append((void **)&buf->data, &buf->len, &buf->cap, sizeof(buf->data[0]), &value) != 0) {
  return ERR_OOM;
}
```

## Variations

- **Growth policy**: doubling (`cap *= 2`), 1.5x growth, fixed chunk growth, or minimum threshold bootstrap.
- **Write mode**: direct assignment for POD types vs `memcpy` for generic element type.
- **Error path**: return codes, `abort`, global error flags, or caller-provided allocator hooks.
- **Index discipline**: pre-increment vs post-increment ordering around element write.
- **Resize strategy**: in-place `realloc` vs allocate-copy-free helper (`malloc` + `memcpy` + `free`) as seen in parson.
- **Operation split**: append logic split across `add()` and `resize()` helpers rather than one local block.

## Edge Cases

- Allocation failure must not clobber original buffer pointer.
- `cap * elem_size` may overflow and must be checked.
- `cap == 0` bootstrap behavior differs across codebases.
- Append of types with ownership/destructor semantics may require deep copy rules.

## Notes

- The append pattern is often split across helper functions and macros, which makes syntactic matching brittle.
- Normalization should treat growth + write + length update as a single semantic unit.
- Follow-up subpatterns worth separating: reserve-only, push-many, and shrink-to-fit.
- In `parson`, append is semantically vector push even though growth happens through a separate copy-resize function.

## Potential Rust Mapping

- Candidate Rust abstraction(s): `Vec<T>`
- Suggested translation shape: `vec.push(value)` (or `vec.extend_from_slice(...)` for batch append)
- Safety considerations:
  - Preserve OOM/error semantics when C code handles allocation failure explicitly.
  - Ensure ownership semantics are preserved for non-`Copy` translations.
