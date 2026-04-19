#include <stddef.h>
#include <stdlib.h>

#define INI_MAX_LINE ((size_t)1024)

int inih_buffer_resize(char **line, size_t *max_line, size_t offset) {
  char *new_line;

  while (*max_line < INI_MAX_LINE && offset == *max_line - 1 &&
         (*line)[offset - 1] != '\n') {
    *max_line *= 2;
    if (*max_line > INI_MAX_LINE) {
      *max_line = INI_MAX_LINE;
    }

    new_line = (char *)realloc(*line, *max_line);
    if (!new_line) {
      free(*line);
      return -2;
    }
    *line = new_line;
  }

  return 0;
}
