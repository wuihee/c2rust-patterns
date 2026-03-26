# Pattern: Stream Read To Buffer

## Name

`stream_read_to_buffer`

## Semantic Intent

Read all bytes from a stream/file descriptor into a growable contiguous byte buffer until EOF.

## Observed In (projects/files)

- Common in CLI tools and parsers that ingest whole files before parsing.
- Common in serialization/deserialization code paths that read unknown-length payloads.

Concrete example for this repository:

- `examples/demo_project/stream_read_to_buffer.c`

## Canonical Code Shape

```c
int __read_all(FILE *f, unsigned char **buf, size_t *len, size_t *cap) {
    unsigned char temp[1024];

    while (1) {
        size_t n = fread(temp, 1, sizeof(temp), f);
        if (n == 0) {
            if (ferror(f)) return -1;
            break; // EOF
        }

        if (__vec_extend_bytes((void **)buf, len, cap, temp, n) != 0) {
            return -1;
        }
    }

    return 0;
}
```

Candidate normalized call site form:

```c
if (__read_all(f, (unsigned char **)&buf, &len, &capacity) != 0) {
    return ERR_IO_OR_OOM;
}
```

## Variations

- **Read primitive**: `fread`, `read`, `recv`, or custom buffered reader wrapper.
- **Chunk size**: fixed stack temp buffer (256/1024/4096 bytes) vs dynamic chunking.
- **Growth logic**: direct capacity doubling in loop vs delegated byte-append helper.
- **Termination/error checks**: `n == 0` + `feof/ferror`, negative return from `read`, retry on `EINTR`.

## Edge Cases

- Partial reads are normal and must not be treated as EOF unless `n == 0`.
- Capacity growth can require multiple expansions for large `n` chunks.
- `len + n` and `new_cap * elem_size` overflow checks are required.
- `realloc` failure must preserve the original buffer pointer.
- Error semantics differ between stdio (`ferror`) and syscall-based loops (`errno`).

## Notes

- This pattern composes two semantic units: stream read and dynamic byte-buffer extend.
- Normalization may be cleaner if `__read_all` is defined on top of a generic `__vec_extend_bytes` intrinsic.
- Many implementations inline growth checks in the read loop, which hides intent from syntactic translators.

## Potential Rust Mapping

- Candidate Rust abstraction(s): `Vec<u8>`, `std::io::Read`
- Suggested translation shape:

```rust
use std::io::Read;

let mut buf = Vec::with_capacity(1024);
reader.read_to_end(&mut buf)?;
```

- Equivalent explicit loop (when preserving chunking behavior is important):

```rust
use std::io::Read;

let mut buf = Vec::with_capacity(1024);
let mut temp = [0u8; 1024];

loop {
    let n = reader.read(&mut temp)?;
    if n == 0 {
        break;
    }
    buf.extend_from_slice(&temp[..n]);
}
```

- Safety considerations:
  - Preserve I/O error behavior (`ferror`/`errno`) instead of silently truncating on error.
  - Preserve OOM behavior if original C path handles allocation failures explicitly.
