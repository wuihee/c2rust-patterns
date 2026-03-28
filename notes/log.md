# Log

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
- Files inspected: `parson/parson.c`
- Suspected pattern: dynamic array append (`json_array_add` + `json_array_resize`)
- Why it matters: semantic vector push is split across helpers and uses allocate-copy-free resize instead of `realloc`.

## 2026-03-25: inih parser patterns

- Date: 2026-03-25
- Repository: inih
- Files inspected: `repos/inih/ini.c`, `repos/inih/cpp/INIReader.cpp`
- Suspected patterns:
  - line reader with realloc growth (`ini.c` read loop with `INI_ALLOW_REALLOC`)
  - overlong-line discard loop (`abyss` chunk drain until newline)
  - in-place trim/split for `name[=:]value`
  - callback-driven parse event loop
  - C++ map entry append (`_values[key] += ...`)
- Why it matters: feature toggles expose controlled variations of the same semantic behavior, useful for normalization rules.
- Open questions: whether line-growth should be a separate pattern or modeled as a specialization of stream-read-to-buffer.
