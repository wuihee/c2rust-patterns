# Pattern: `vec_push`

## Semantic Intent

Append one logical element to a contiguous growable buffer and increment length. Capacity growth is treated as a separate concern (for example, `vec_grow`).

## Observed In

- [`parson`](../examples/parson/vec_push.c)

## Variations

- **Index Discipline**: write-then-increment (`items[len] = x; len++`) vs increment-then-write patterns.
- **Error Path Coupling**: push may call grow helper and bubble failure, or assume caller already reserved capacity.

## Edge Cases

- Caller must ensure spare capacity before write; otherwise push may write out of bounds.
- Ownership/aliasing rules for element payloads may require deep-copy behavior in some codebases.

## Notes

## Canonical Code Shape

```c
int __vec_push(void *data, size_t *len, size_t cap, size_t elem_size, const void *elem) {
  if (*len >= cap) {
    return -1;
  }

  memcpy((char *)data + (*len * elem_size), elem, elem_size);
  *len += 1;
  return 0;
}
```

## Potential Rust Mapping

- Candidate Rust abstraction(s): `Vec<T>`
- Safety considerations:
  - Preserve push failure behavior when C path exposes explicit allocation/error states.
  - Preserve ownership semantics for non-`Copy` values during translation.
