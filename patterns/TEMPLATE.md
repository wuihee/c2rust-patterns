# Pattern Name: `<pattern_name>`

Describe the high-level operation this pattern performs and its semantic intent.

## Observed In

## Variations

## Edge Cases

- Allocation failure handling:
- Integer overflow in size/capacity math:
- Null pointer behavior:
- Ownership/lifetime assumptions:

## Notes

Free-form observations, unresolved questions, and implementation details that may affect normalization.

## Canonical Code Shape

```c
// Pseudocode or normalized C form
if (len == cap) {
  cap = grow(cap);
  data = realloc(data, cap * elem_size);
}

write(data, len, value);
len += 1;
```

Optionally define a candidate intrinsic/API call:

```c
__vec_append(&data, &len, &cap, value, elem_size);
```

## Potential Rust Mapping

- Candidate Rust abstraction(s):
- Suggested translation shape:
- Safety considerations:
