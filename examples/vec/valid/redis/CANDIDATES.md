# Redis Dynamic Array Candidates

Wide-net exploration. False positives expected.

---

## 1. `vec` — `src/vector.h` + `src/vector.c`

Classic ptr+len+cap struct. Push-single with custom-grow.

```c
// src/vector.h:58-63
typedef struct vec {
    size_t size;   /* len */
    size_t cap;    /* capacity */
    void **data;   /* heap storage or stack ptr */
    void **stack;  /* optional stack buffer */
} vec;
```

```c
// src/vector.c:82-88 — push-single + custom-grow
void vecPush(vec *v, void *value) {
    if (unlikely(v->size == v->cap)) {
        size_t newcap = (v->cap > 0) ? v->cap * 2 : VEC_DEFAULT_INITCAP;
        vecReserve(v, newcap);
    }
    v->data[v->size++] = value;
}
```

```c
// src/vector.c:64-79 — custom grow (stack spill + zrealloc)
void vecReserve(vec *v, size_t mincap) {
    if (mincap <= v->cap) return;
    if (v->data == v->stack) {
        newdata = zmalloc(mincap * sizeof(void *));
        if (v->size) memcpy(newdata, v->data, v->size * sizeof(void *));
    } else {
        newdata = zrealloc(v->data, mincap * sizeof(void *));
    }
    v->data = newdata;
    v->cap = mincap;
}
```

**Tags:** `struct-based`, `push-single`, `custom-grow`
**Notable:** stack-buffer fallback (small-vec optimization)

---

## 2. `sdshdr{8,16,32,64}` — `src/sds.h` + `src/sds.c`

Inline-header dynamic string. `buf[]` is flexible array tail; `len`=used, `alloc`=capacity (excluding header+null).

```c
// src/sds.h:32-37
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len;           /* used */
    uint8_t alloc;         /* capacity (excl. header + null) */
    unsigned char flags;
    char buf[];
};
```

Growth function (`src/sds.c:269-327`) selects header type by size, then `s_realloc_usable` or `s_malloc_usable` + `memcpy` when header type changes:

```c
sds _sdsMakeRoomFor(sds s, size_t addlen, int greedy) {
    // greedy=1: double until SDS_MAX_PREALLOC, then add MAX_PREALLOC
    if (greedy == 1) {
        if (newlen < SDS_MAX_PREALLOC) newlen *= 2;
        else newlen += SDS_MAX_PREALLOC;
    }
    // realloc in-place if header type unchanged, else malloc+memcpy+free
    ...
}
```

**Tags:** `struct-based` (inline header), `extend`, `custom-grow`
**Notable:** 4 size variants (uint8/16/32/64); pointer arithmetic to access header; no split-args

---

## 3. `aeEventLoop` events/fired arrays — `src/ae.h` + `src/ae.c`

Two parallel dynamic arrays (`events`, `fired`) sized by `nevents` (len) with `setsize` as a cap ceiling.

```c
// src/ae.h:79-93
typedef struct aeEventLoop {
    int setsize;          /* max capacity ceiling */
    int nevents;          /* current allocated count (len) */
    aeFileEvent *events;  /* ptr */
    aeFiredEvent *fired;  /* ptr */
    ...
} aeEventLoop;
```

```c
// src/ae.c:154-165 — grow on demand (doubles, capped at setsize)
if (unlikely(fd >= eventLoop->nevents)) {
    int newnevents = eventLoop->nevents;
    newnevents = (newnevents * 2 > fd + 1) ? newnevents * 2 : fd + 1;
    newnevents = (newnevents > eventLoop->setsize) ? eventLoop->setsize : newnevents;
    eventLoop->events = zrealloc(eventLoop->events, sizeof(aeFileEvent) * newnevents);
    eventLoop->fired  = zrealloc(eventLoop->fired,  sizeof(aeFiredEvent) * newnevents);
    eventLoop->nevents = newnevents;
}
```

```c
// src/ae.c:107-122 — resize (can shrink)
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize) {
    if (setsize < eventLoop->nevents) {
        eventLoop->events = zrealloc(eventLoop->events, sizeof(aeFileEvent)*setsize);
        eventLoop->fired  = zrealloc(eventLoop->fired,  sizeof(aeFiredEvent)*setsize);
        eventLoop->nevents = setsize;
    }
    ...
}
```

**Tags:** `struct-based`, two parallel arrays, capped growth
**Notable:** Separate `setsize` ceiling vs `nevents` length; not pure push — indexed by fd

---

## 4. `raxStack` — `src/rax.h` + `src/rax.c`

Stack-with-fallback pattern (same as `vec` but pointer-only, uses static inline buffer).

```c
// src/rax.h:124-132
#define RAX_STACK_STATIC_ITEMS 32
typedef struct raxStack {
    void **stack;                            /* ptr (static or heap) */
    size_t items, maxitems;                  /* len, cap */
    void *static_items[RAX_STACK_STATIC_ITEMS]; /* inline buffer */
    int oom;
} raxStack;
```

```c
// src/rax.c:75-93 — push-single with stack-spill then realloc
static inline int raxStackPush(raxStack *ts, void *ptr) {
    if (ts->items == ts->maxitems) {
        if (ts->stack == ts->static_items) {
            ts->stack = rax_malloc(sizeof(void*)*ts->maxitems*2);
            memcpy(ts->stack, ts->static_items, sizeof(void*)*ts->maxitems);
        } else {
            void **newalloc = rax_realloc(ts->stack, sizeof(void*)*ts->maxitems*2);
            ts->stack = newalloc;
        }
        ts->maxitems *= 2;
    }
    ts->stack[ts->items++] = ptr;
    return 1;
}
```

**Tags:** `struct-based`, `push-single`, `custom-grow`, stack-spill
**Notable:** Near-identical pattern to `vec`; doubles on overflow

---

## 5. `quicklist` bookmark flexible array — `src/quicklist.h`

`quicklist` has a flexible array tail of `quicklistBookmark`. Count stored in `bm_count` (4 bits, max 16).

```c
// src/quicklist.h (approx lines 79-117)
typedef struct quicklist {
    quicklistNode *head;
    quicklistNode *tail;
    unsigned long count;
    unsigned long len;
    ...
    unsigned int bm_count : 4;  /* bookmark count */
    quicklistBookmark bookmarks[];
} quicklist;
```

Growth: `quicklistBookmarkCreate` calls `zrealloc(ql, sizeof(*ql) + (ql->bm_count+1)*sizeof(quicklistBookmark))`.

**Tags:** flexible-array tail, realloc-whole-struct
**Notable:** Struct itself is reallocated to extend tail

---

## 6. `listpack` / `ziplist` opaque buffer — `src/listpack.h`, `src/ziplist.h`

Single `unsigned char *` blob that grows/shrinks via `lp_realloc` / `realloc`. Length encoded in first 6 bytes; no separate len/cap fields.

```c
// Growth pattern (listpack.c)
unsigned char *lpInsert(...) {
    ...
    unsigned char *newlp = lp_realloc(lp, curlen + room_for_new);
    ...
}
```

**Tags:** opaque-buffer, no explicit cap field
**Likely invalid:** missing-capacity (realloc to exact needed size each time)

---

## Summary Table

| Candidate          | File(s)       | Ptr field        | Len field         | Cap field  | Growth fn                     | Tags                                   |
| ------------------ | ------------- | ---------------- | ----------------- | ---------- | ----------------------------- | -------------------------------------- |
| `vec`              | vector.h/c    | `data`           | `size`            | `cap`      | `vecReserve`                  | struct-based, push-single, custom-grow |
| `sdshdr8/16/32/64` | sds.h/c       | `buf[]` (inline) | `len`             | `alloc`    | `_sdsMakeRoomFor`             | inline-header, extend, custom-grow     |
| `aeEventLoop`      | ae.h/c        | `events`/`fired` | `nevents`         | `setsize`  | inline in `aeCreateFileEvent` | struct-based, capped, parallel arrays  |
| `raxStack`         | rax.h/c       | `stack`          | `items`           | `maxitems` | `raxStackPush`                | struct-based, push-single, stack-spill |
| `quicklist` bm     | quicklist.h/c | `bookmarks[]`    | `bm_count`        | —          | `quicklistBookmarkCreate`     | flex-array-tail                        |
| `listpack`         | listpack.h/c  | opaque `lp` ptr  | encoded in header | none       | `lp_realloc`                  | opaque-buffer, likely invalid          |
