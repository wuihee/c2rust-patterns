Goal

- Build a high-signal pattern catalog from real C code that cleanly maps to idiomatic Rust.
- End month 1 with: ~10 strong patterns, cross-repo evidence, draft canonical intrinsics, and 1-2 prototype normalization targets.
  Repo Order (Why This Sequence)
- Week 1: git/git, curl/curl (clean, recurring buffer/string/IO idioms)
- Week 2: redis/redis, libgit2/libgit2 (hash maps, SDS/string, robust error/alloc paths)
- Week 3: nginx/nginx, libuv/libuv (arena/pool, arrays, intrusive structures)
- Week 4: sqlite/sqlite or FFmpeg/FFmpeg + glib (complex edge cases + explicit C abstractions for comparison)
  Top 10 Patterns To Prioritize
- dynamic_array_append -> Vec::push / extend_from_slice
- stream_read_to_buffer -> Read::read_to_end
- string_builder_append -> String::push_str / Vec<u8>
- string_builder_printf -> write! / format! + append
- hashmap_lookup_insert_update -> HashMap::entry
- hashmap_resize_rehash -> HashMap growth semantics
- reserve_then_bulk_copy -> Vec::reserve + extend_from_slice
- arena_alloc_lifetime_region -> scoped ownership / bump alloc equivalent
- goto_cleanup_error_ladder -> RAII + Result propagation
- intrusive_list_push_pop -> ownership-safe linked/list abstractions (or VecDeque where semantically valid)
  Weekly Execution Plan
- Week 1 (Foundation + fast wins)
  - Mine git + curl for 3 patterns: array append, stream read, string builder append.
  - Deliverables:
    - 3 completed patterns/\*.md
    - 2-3 examples per pattern from different projects
    - initial canonical forms (**vec_append, **read_all, \_\_str_append_bytes)
- Week 2 (Maps + mutation semantics)
  - Mine redis + libgit2 for map/set and reserve/copy idioms.
  - Deliverables:
    - 3 more patterns: map insert/update, rehash, bulk append
    - explicit variation matrix (collision handling, growth policy, error paths)
    - draft Rust mapping notes with edge-case behavior preservation
- Week 3 (Memory model patterns)
  - Mine nginx + libuv for pool/arena and intrusive structures.
  - Deliverables:
    - 2 patterns: arena region alloc, intrusive list ops
    - “translation risk” notes (lifetime/aliasing/ownership mismatch points)
    - candidate decomposition rules (when to map to std types vs custom wrappers)
- Week 4 (Hard cases + synthesis)
  - Mine sqlite or FFmpeg; use glib as contrast corpus.
  - Deliverables: - final 2 patterns (cleanup ladder + advanced buffer/string formatting) - v0.1 canonical intrinsic surface (8-12 operations) - shortlist of 1-2 patterns for prototype normalization pass
    Per-Pattern Acceptance Criteria (Quality Bar)
- At least 2 repositories confirm recurrence.
- Includes:
  - semantic intent
  - canonical code shape
  - 3+ implementation variations
  - edge cases + failure semantics
  - idiomatic Rust target + caveats
- Has one minimized extraction with provenance metadata.
  Evidence Capture Template (per example)
- Upstream repo URL
- Commit SHA
- Original file path + line anchors
- Why this snippet matches pattern
- Notes on behavior that must be preserved in Rust
  Milestones
- End of Week 2: 6 solid patterns, enough to define initial intrinsic naming conventions.
- End of Week 4: 10 patterns + v0.1 normalization API + prototype target list.
  Suggested Next Step

1. I can now draft the exact v0.1 canonical intrinsic list (names, signatures, semantics) to match this 4-week plan
