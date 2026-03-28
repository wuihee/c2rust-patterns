# Pattern: `vec_grow`

## Semantic Intent

Ensure a growable contiguous buffer has enough spare capacity for an upcoming write (one element or many bytes), without changing logical length.

## Observed In

- [`parson`](../examples/parson/vec_grow.c)

## Variations

- **Growth Policy**: doubling, 1.5x, fixed increment, capped growth.
- **Resize Strategy**: in-place `realloc` vs allocate-copy-free helper (`malloc` + `memcpy` + `free`).
- **Trigger Condition**: `len == cap`, `len + n > cap`, or line-truncation checks (`offset == max_line - 1 && ... != '\n'`).

## Edge Cases

## Notes

- Do we keep this pattern separate from appending to a vec?
  - On one hand, reserving capacity or growing a vec always accompanies an append.
  - However, in C code, `vec_grow` is a lot of times its own abstraction, coded separately from vec append.

## Canonical Code Shape

```c
int __vec_grow(void **data, size_t *cap, size_t elem_size, size_t min_needed) {
  if (*cap >= min_needed) {
    return 0;
  }

  size_t new_cap = (*cap == 0) ? 8 : *cap;

  while (new_cap < min_needed) {
    size_t prev = new_cap;
    new_cap *= 2;

    // overflow guard
    if (new_cap < prev){
      return -1;
    }
  }

  void *new_data = realloc(*data, new_cap * elem_size);

  if (!new_data) {
    return -1;
  }

  *data = new_data;
  *cap = new_cap;
  return 0;
}
```

## Potential Rust Mapping

- Candidate Rust abstraction(s): `Vec<T>`
- Safety considerations:
  - Preserve explicit allocation failure/error semantics where C code expects error returns.
  - Preserve upper-bound checks when C logic enforces hard limits.
