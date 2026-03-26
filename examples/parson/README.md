# parson example metadata

## Upstream

- Repository: `https://github.com/kgabis/parson`
- Project: `parson` (single-file JSON parser in C)
- License: MIT (verify against upstream at selected commit)

## Snapshot provenance

- Upstream file used: `parson.c`
- Commit SHA: `<fill-me>`
- Extraction date: `2026-03-25`
- Extraction method: manual minimization from upstream array append path

## Included local artifacts

- `examples/parson/dynamic_array_append.c`
  - Minimized from upstream `json_array_add` and `json_array_resize`
  - Captures: grow-on-full, copy-resize (`malloc` + `memcpy` + `free`), append, count increment

## Pattern links

- Primary pattern: `patterns/dynamic_array_append.md`
- Related notes: `notes/research-log.md`

## Notes

- This example intentionally keeps only the structural logic needed for dynamic array append analysis.
- It is not intended to be a buildable standalone subset of upstream parson.
