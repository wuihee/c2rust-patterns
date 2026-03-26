# Methodology

## 1) Pattern Discovery

- Inspect real-world C repositories.
- Focus on repeated low-level idioms that imply higher-level behavior.
- Collect representative snippets and source locations.

## 2) Pattern Documentation

- Use `patterns/TEMPLATE.md` for consistency.
- Record semantic intent, canonical shape, variants, and edge cases.
- Link evidence in `examples/<project>/`.

## 3) Normalization Design

- Propose canonical intrinsics or C API forms (for example, `__vec_append`).
- Keep API surface minimal and semantically explicit.
- Document assumptions and failure semantics.

## 4) Rust Mapping

- Map canonical forms to idiomatic Rust constructs (`Vec`, `String`, `HashMap`, etc.).
- Note ownership, lifetime, and error-handling implications.

## 5) Iteration

- Refine patterns as more codebases are studied.
- Split/merge patterns when needed to maintain semantic clarity.
