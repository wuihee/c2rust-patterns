// repos/inih/ini.c

#include <stddef.h>
#include <stdlib.h>

#define INI_MAX_LINE ((size_t)1024)

// [Pointer] — line
// [Capacity] — max_line
// [Length: Missing] — no length field
// [Push Value: Stream] — reader() fills the buffer directly from a stream
// rather than copying from an in-memory buffer; fails vec because push reads
// from an external source.
int ini_parse_stream(ini_reader reader, void *stream, ini_handler handler,
                     void *user) {
  char *line;
  size_t max_line = INI_INITIAL_ALLOC;
  char *new_line;
  size_t offset;
  int error = 0;

  line = (char *)ini_malloc(INI_INITIAL_ALLOC);
  if (!line)
    return -2;

  while (reader(line, (int)max_line, stream) != NULL) { // [Push Value: Stream]
    offset = strlen(line);

    while (max_line < INI_MAX_LINE && offset == max_line - 1 &&
           line[offset - 1] != '\n') {
      max_line *= 2; // [Ensure Capacity]
      if (max_line > INI_MAX_LINE)
        max_line = INI_MAX_LINE;
      new_line = ini_realloc(line, max_line);
      if (!new_line) {
        ini_free(line);
        return -2;
      }
      line = new_line;
      if (reader(line + offset, (int)(max_line - offset), stream) ==
          NULL) // [Push Value: Stream]
        break;
      offset += strlen(line + offset);
    }
  }

  ini_free(line);
  return error;
}
