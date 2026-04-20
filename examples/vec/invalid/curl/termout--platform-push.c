// repos/curl/src/tool_cb_wrt.c

#ifdef _WIN32
#include <stddef.h>
#include <stdlib.h>
#include <windows.h>

struct termout {
  // [Pointer]
  wchar_t *buf;

  // [Length] — tracks the current allocated slot count, not a byte length.
  // Used both as capacity sentinel and as the length passed to WriteConsoleW.
  DWORD len;
};

static size_t win_console(struct termout *term,
                          const unsigned char *rbuf, DWORD rlen) {
  // Compute how many wide chars the UTF-8 input will expand to.
  DWORD len = (DWORD)MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)rbuf, (int)rlen,
                                         NULL, 0);
  if (!len)
    return 0;

  // [Ensure Capacity]
  if (len > term->len) {
    wchar_t *buf = realloc(term->buf, len * sizeof(wchar_t));
    if (!buf)
      return 0;
    term->len = len;
    term->buf = buf;
  }

  // [Push Value: Platform API] — conversion + write in one call via
  // MultiByteToWideChar; no memcpy or direct assignment.
  len = (DWORD)MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)rbuf, (int)rlen,
                                   term->buf, (int)len);
  if (!len)
    return 0;

  DWORD chars_written;
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), term->buf, len,
                &chars_written, NULL);
  return rlen;
}
#endif /* _WIN32 */
