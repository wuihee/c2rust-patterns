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

- Add 5-10 high-frequency patterns from diverse C codebases.
- Compare implementation variants and failure modes across projects.
- Draft first version of canonical intrinsic/API surface.
- Prototype a small normalization pass for one or two patterns.

### Notes

It seems like `buffer_resize` is a _semantic primitve_ built from many _operational primitives_.

### Potential Test Cases

- git
- curl
- redis
- libgit2
- nginx
- libuv
- sqlite
- ffmpeg
