#include <stddef.h>
#include <stdlib.h>

#define INI_MAX_LINE ((size_t)1024)

int inih_buffer_resize(char **line, size_t *max_line, size_t offset) {
  // ...

  char *new_line;

  // Capacity Guard
  while (*max_line < INI_MAX_LINE && offset == *max_line - 1 &&
         (*line)[offset - 1] != '\n') {
    // Growth Policy
    *max_line *= 2;
    if (*max_line > INI_MAX_LINE) {
      *max_line = INI_MAX_LINE;
    }

    // Reallocation
    new_line = (char *)realloc(*line, *max_line);
    if (!new_line) {
      free(*line);
      return -2;
    }

    // State Update
    *line = new_line;
  }

  // Notice how we don't have a "state update" or a "capacity" state.

  // ...

  return 0;
}
