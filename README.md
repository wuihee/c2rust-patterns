# c2rust-patterns

Research repository for improving idiomatic C-to-Rust translation through pattern discovery and normalization.

## Motivation

Most C-to-Rust translators (e.g. c2rust) operate mostly at the syntactic level. Real C code, however, often encodes higher-level abstractions implicitly across multiple low-level operations.

For example, what is semantically a vector append may appear as:

- capacity check
- `realloc` growth
- `memcpy` or direct write
- length increment

When these operations are treated as unrelated statements, translation quality suffers and generated Rust code is often non-idiomatic. This project studies recurring implementation patterns in real C code and normalizes them into canonical representations that can map cleanly to Rust abstractions such as `Vec`, `String`, and `HashMap`.

## Project Approach

The workflow is intentionally simple and iterative:

1. **Pattern discovery**: inspect real-world C codebases and collect recurring low-level implementation patterns.
2. **Pattern documentation**: record semantic intent, code shape, variations, and edge cases in a shared catalog.
3. **Normalization design**: define canonical intrinsics or a small C API (e.g., `__vec_append`) to represent each pattern semantically.
4. **Translation mapping**: map canonical forms to idiomatic Rust constructs.

## Repository Structure

```text
.
├── docs/                  # project overview, methodology, and process docs
├── examples/              # extracted real-world snippets grouped by source project
├── notes/                 # raw observations, open questions, and work logs
├── patterns/              # one markdown file per documented pattern
└── tools/                 # future scripts (analysis, extraction, normalization)
```

## Current Status

This repository is in the **early pattern discovery phase**.

### TODOs

- Add 5-10 high-frequency patterns from diverse C codebases.
- Compare implementation variants and failure modes across projects.
- Draft first version of canonical intrinsic/API surface.
- Prototype a small normalization pass for one or two patterns.
