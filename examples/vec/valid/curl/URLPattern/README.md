# cURL

## `tool_urlglob.c`

`add_glob` ensures capacity after `glob_fixed`, `glob_set`, or `glob_range` pushes a new value into the array.

### Vec

- `struct URLPattern`
- `struct set`

### Code

- `glob_parse`: Converts a URL string into a sequence of `URLPattern`s.

```c
/* Represents the program state. */
struct URLGlob {
  /* String builder for output. */
  struct dynbuf buf;

  /* Array of patterns. */
  struct URLPattern *pattern;

  /* Capacity */
  size_t palloc;

  /* Number of pattners (length) */
  size_t pnum;

  /* Iteration state */
  char beenhere;

  const char *error;
  size_t pos;
};

typedef enum { GLOB_SET = 1, GLOB_ASCII, GLOB_NUM } globtype;

/* Represents a single segment of the URL that can produce values.
 *
 * Each URLPattern acts as a generator that can produce one value at a time.
 */
struct URLPattern {
  globtype type;
  int globindex; /* the number of this particular glob or -1 if not used
                    within {} or [] */
  union {
    /* E.g. {a,b,c} */
    struct {
      char **elem;
      curl_off_t size;
      curl_off_t idx;
      size_t palloc;
    } set;

    /* E.g. [a-z] */
    struct {
      int min;
      int max;
      int letter;
      unsigned char step;
    } ascii;

    /* E.g. [1-10] */
    struct {
      curl_off_t min;
      curl_off_t max;
      curl_off_t idx;
      curl_off_t step;
      int npad;
    } num;
  } c;
};
```
