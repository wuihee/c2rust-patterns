# c2rust-patterns

Research repository for improving idiomatic C-to-Rust translation through pattern discovery and normalization.

## Motivation

Most C-to-Rust translators (e.g. c2rust) operate mostly at the syntactic level. Real C code, however, often encodes higher-level abstractions implicitly across multiple low-level operations.

For example, what is semantically a vector append may appear as:

- capacity check
- `realloc` growth
- `memcpy` or direct write
- length increment

When these operations are treated as unrelated statements, translation quality suffers and generated Rust code is often non-idiomatic.

This project studies recurring patterns in real C code that can map cleanly to Rust's `Vec` abstraction.

## Repository Structure

```text
.
├── docs/                  # project overview, methodology, and process docs
├── examples/              # extracted real-world snippets grouped by source project
├── notes/                 # raw observations, open questions, and work logs
├── patterns/              # catalog of common C patterns
└── tools/                 # future scripts (analysis, extraction, normalization)
```

## TODOs

- Identify the individual, identifying components of `vec` transitions.

### Potential Test Cases

- git
- redis
- libgit2
- nginx
- libuv
- sqlite
- ffmpeg
