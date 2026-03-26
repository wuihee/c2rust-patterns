# Research Log

Use this file for quick, raw observations during codebase study.

## Entry Template

- Date:
- Repository:
- Files inspected:
- Suspected pattern:
- Why it matters:
- Open questions:

## 2026-03-25: parson JSON array append

- Date: 2026-03-25
- Repository: parson
- Files inspected: `examples/parson/parson.c`
- Suspected pattern: dynamic array append (`json_array_add` + `json_array_resize`)
- Why it matters: semantic vector push is split across helpers and uses allocate-copy-free resize instead of `realloc`, which is easy to miss with purely local matching.
- Open questions: should normalization model this as `__vec_append` directly, or first normalize `json_array_resize` into a generic `__vec_reserve` primitive and fold at a later pass?
