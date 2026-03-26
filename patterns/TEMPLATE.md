# Pattern: <name>

## Name

`<pattern_identifier>`

## Semantic Intent

Describe the high-level operation this pattern performs (for example: append element to growable buffer, insert/update key-value pair, reserve capacity).

## Observed In (projects/files)

- `<project>`: `<path/to/file.c>`
- `<project>`: `<path/to/another_file.c>`

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

## Variations

- Variation 1:
- Variation 2:
- Variation 3:

## Edge Cases

- Allocation failure handling:
- Integer overflow in size/capacity math:
- Null pointer behavior:
- Ownership/lifetime assumptions:

## Notes

Free-form observations, unresolved questions, and implementation details that may affect normalization.

## Potential Rust Mapping

- Candidate Rust abstraction(s):
- Suggested translation shape:
- Safety considerations:
