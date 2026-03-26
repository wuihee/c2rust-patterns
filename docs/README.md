# Project Overview

This project investigates how to improve C-to-Rust translation quality by recognizing implicit high-level abstractions in low-level C implementations.

Instead of translating raw syntax directly, we aim to:

1. Identify recurring implementation patterns in production C code.
2. Normalize each pattern to a canonical representation.
3. Translate canonical forms to idiomatic Rust abstractions.

See `docs/methodology.md` for process details and `patterns/` for the evolving catalog.
