// repos/curl/tests/server/getpart.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  GPE_OK = 0,
  GPE_OUT_OF_MEMORY = 1,
} getpart_error;

/*
 * appenddata()
 *
 * This appends data from a given source buffer to the end of the used part of
 * a destination buffer. Arguments relative to the destination buffer are, the
 * address of a pointer to the destination buffer 'dst_buf', the length of data
 * in destination buffer excluding potential null string termination 'dst_len',
 * the allocated size of destination buffer 'dst_alloc'. All three destination
 * buffer arguments may be modified by this function. Arguments relative to the
 * source buffer are, a pointer to the source buffer 'src_buf' and indication
 * whether the source buffer is base64 encoded or not 'src_b64'.
 *
 * If the source buffer is indicated to be base64 encoded, this appends the
 * decoded data, binary or whatever, to the destination. The source buffer
 * may not hold binary data, only a null-terminated string is valid content.
 *
 * Destination buffer will be enlarged and relocated as needed.
 *
 * Calling function is responsible to provide preallocated destination
 * buffer and also to deallocate it when no longer needed.
 *
 * This function may return:
 *   GPE_OUT_OF_MEMORY
 *   GPE_OK
 */
static int
appenddata(char **dst_buf,      /* [Pointer] dest buffer */
           size_t *dst_len,     /* [Length] dest buffer data length */
           size_t *dst_alloc,   /* [Capacity] dest buffer allocated size */
           const char *src_buf, /* source buffer */
           size_t src_len,      /* source buffer length */
           int src_b64)         /* != 0 if source is base64
                                   encoded */
{
  size_t need_alloc = 0;

  if (!src_len)
    return GPE_OK;

  need_alloc = src_len + *dst_len + 1;

  if (src_b64) {
    if (src_buf[src_len - 1] == '\r')
      src_len--;

    if (src_buf[src_len - 1] == '\n')
      src_len--;
  }

  // [Ensure Capacity]
  /* enlarge destination buffer if required */
  if (need_alloc > *dst_alloc) {
    size_t newsize = need_alloc * 2;
    char *newptr = realloc(*dst_buf, newsize);
    if (!newptr) {
      return GPE_OUT_OF_MEMORY;
    }
    *dst_alloc = newsize;
    *dst_buf = newptr;
  }

  // [Push Value]
  /* memcpy to support binary blobs */
  memcpy(*dst_buf + *dst_len, src_buf, src_len);
  *dst_len += src_len;
  *(*dst_buf + *dst_len) = '\0';

  return GPE_OK;
}
