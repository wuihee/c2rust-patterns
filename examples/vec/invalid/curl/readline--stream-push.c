// repos/curl/tests/server/getpart.c

/*
 * readline()
 *
 * Reads a complete line from a file into a dynamically allocated buffer.
 *
 * Calling function may call this multiple times with same 'buffer'
 * and 'bufsize' pointers to avoid multiple buffer allocations. Buffer
 * will be reallocated and 'bufsize' increased until whole line fits in
 * buffer before returning it.
 *
 * Calling function is responsible to free allocated buffer.
 *
 * This function may return:
 *   GPE_OUT_OF_MEMORY
 *   GPE_END_OF_FILE
 *   GPE_OK
 */
static int readline(char **buffer,   // [Vec Pointer]
                    size_t *bufsize, // [Capacity]
                    size_t *length,  // [Length]
                    FILE *stream) {
  size_t offset = 0;
  char *newptr;

  if (!*buffer) {
    *buffer = calloc(1, 128);
    if (!*buffer)
      return GPE_OUT_OF_MEMORY;
    *bufsize = 128;
  }

  for (;;) {
    int bytestoread = curlx_uztosi(*bufsize - offset);

    // [Push Value: Invalid]
    if (!fgets(*buffer + offset, bytestoread, stream)) {
      *length = 0;
      return (offset != 0) ? GPE_OK : GPE_END_OF_FILE;
    }

    *length = offset + line_length(*buffer + offset, bytestoread);
    if (*(*buffer + *length - 1) == '\n')
      break;
    offset = *length;
    if (*length < *bufsize - 1)
      continue;

    newptr = realloc(*buffer, *bufsize * 2);
    if (!newptr)
      return GPE_OUT_OF_MEMORY;
    memset(&newptr[*bufsize], 0, *bufsize);
    *buffer = newptr;
    *bufsize *= 2;
  }

  return GPE_OK;
}
