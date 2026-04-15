
#ifdef _WIN32
struct termout {
  // [Vec Pointer]
  wchar_t *buf;

  // [Length]
  DWORD len;
};
#endif

struct GlobalConfig {
  struct State state; /* for create_transfer() */
  char *trace_dump;   /* file to dump the network trace to */
  FILE *trace_stream;
  char *libcurl;      /* Output libcurl code to this filename */
  char *ssl_sessions; /* file to load/save SSL session tickets */
  struct tool_var *variables;
  struct OperationConfig *first;
  struct OperationConfig *current;
  struct OperationConfig *last;
#ifdef _WIN32
  struct termout term;
#endif
  timediff_t ms_per_transfer; /* start next transfer after (at least) this
                                 many milliseconds */
  trace tracetype;
  int progressmode;             /* CURL_PROGRESS_BAR / CURL_PROGRESS_STATS */
  unsigned short parallel_host; /* MAX_PARALLEL_HOST is the maximum */
  unsigned short parallel_max;  /* MAX_PARALLEL is the maximum */
  unsigned char verbosity;      /* How verbose we should be */
#ifdef DEBUGBUILD
  BIT(test_duphandle);
  BIT(test_event_based);
#endif
  BIT(parallel);
  BIT(parallel_connect);
  BIT(fail_early);    /* exit on first transfer error */
  BIT(styled_output); /* enable fancy output style detection */
  BIT(trace_fopened);
  BIT(tracetime);  /* include timestamp? */
  BIT(traceids);   /* include xfer-/conn-id? */
  BIT(showerror);  /* show errors when silent */
  BIT(silent);     /* do not show messages, --silent given */
  BIT(noprogress); /* do not show progress bar */
  BIT(isatty);     /* Updated internally if output is a tty */
  BIT(trace_set);  /* --trace-config has been used */
};

#ifdef _WIN32
static size_t win_console(intptr_t fhnd, struct OutStruct *outs, char *buffer,
                          size_t bytes, size_t *retp) {
  DWORD chars_written;
  unsigned char *rbuf = (unsigned char *)buffer;
  DWORD rlen = (DWORD)bytes;

#define IS_TRAILING_BYTE(x) (0x80 <= (x) && (x) < 0xC0)

  /* attempt to complete an incomplete UTF-8 sequence from previous call. the
     sequence does not have to be well-formed. */
  if (outs->utf8seq[0] && rlen) {
    bool complete = false;
    /* two byte sequence (lead byte 110yyyyy) */
    if (0xC0 <= outs->utf8seq[0] && outs->utf8seq[0] < 0xE0) {
      outs->utf8seq[1] = *rbuf++;
      --rlen;
      complete = true;
    }
    /* three byte sequence (lead byte 1110zzzz) */
    else if (0xE0 <= outs->utf8seq[0] && outs->utf8seq[0] < 0xF0) {
      if (!outs->utf8seq[1]) {
        outs->utf8seq[1] = *rbuf++;
        --rlen;
      }
      if (rlen && !outs->utf8seq[2]) {
        outs->utf8seq[2] = *rbuf++;
        --rlen;
        complete = true;
      }
    }
    /* four byte sequence (lead byte 11110uuu) */
    else if (0xF0 <= outs->utf8seq[0] && outs->utf8seq[0] < 0xF8) {
      if (!outs->utf8seq[1]) {
        outs->utf8seq[1] = *rbuf++;
        --rlen;
      }
      if (rlen && !outs->utf8seq[2]) {
        outs->utf8seq[2] = *rbuf++;
        --rlen;
      }
      if (rlen && !outs->utf8seq[3]) {
        outs->utf8seq[3] = *rbuf++;
        --rlen;
        complete = true;
      }
    }

    if (complete) {
      WCHAR prefix[3] = {0}; /* UTF-16 (1-2 WCHARs) + NUL */

      if (MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)outs->utf8seq, -1, prefix,
                              CURL_ARRAYSIZE(prefix))) {
        DEBUGASSERT(prefix[2] == L'\0');
        if (!WriteConsoleW((HANDLE)fhnd, prefix, prefix[1] ? 2 : 1,
                           &chars_written, NULL)) {
          return CURL_WRITEFUNC_ERROR;
        }
      }
      /* else: UTF-8 input was not well formed and OS is pre-Vista which drops
         invalid characters instead of writing U+FFFD to output. */
      memset(outs->utf8seq, 0, sizeof(outs->utf8seq));
    }
  }

  /* suppress an incomplete utf-8 sequence at end of rbuf */
  if (!outs->utf8seq[0] && rlen && (rbuf[rlen - 1] & 0x80)) {
    /* check for lead byte from a two, three or four byte sequence */
    if (0xC0 <= rbuf[rlen - 1] && rbuf[rlen - 1] < 0xF8) {
      outs->utf8seq[0] = rbuf[rlen - 1];
      rlen -= 1;
    } else if (rlen >= 2 && IS_TRAILING_BYTE(rbuf[rlen - 1])) {
      /* check for lead byte from a three or four byte sequence */
      if (0xE0 <= rbuf[rlen - 2] && rbuf[rlen - 2] < 0xF8) {
        outs->utf8seq[0] = rbuf[rlen - 2];
        outs->utf8seq[1] = rbuf[rlen - 1];
        rlen -= 2;
      } else if (rlen >= 3 && IS_TRAILING_BYTE(rbuf[rlen - 2])) {
        /* check for lead byte from a four byte sequence */
        if (0xF0 <= rbuf[rlen - 3] && rbuf[rlen - 3] < 0xF8) {
          outs->utf8seq[0] = rbuf[rlen - 3];
          outs->utf8seq[1] = rbuf[rlen - 2];
          outs->utf8seq[2] = rbuf[rlen - 1];
          rlen -= 3;
        }
      }
    }
  }

  if (rlen) {
    /* calculate buffer size for wide characters */
    // [Capacity?]
    DWORD len = (DWORD)MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)rbuf, (int)rlen,
                                           NULL, 0);
    if (!len)
      return CURL_WRITEFUNC_ERROR;

    // [Ensure Capacity]
    /* grow the buffer if needed */
    if (len > global->term.len) {
      wchar_t *buf =
          (wchar_t *)curlx_realloc(global->term.buf, len * sizeof(wchar_t));
      if (!buf)
        return CURL_WRITEFUNC_ERROR;
      global->term.len = len;
      global->term.buf = buf;
    }

    // [Weird Push Valud]
    len = (DWORD)MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)rbuf, (int)rlen,
                                     global->term.buf, (int)len);
    if (!len)
      return CURL_WRITEFUNC_ERROR;

    if (!WriteConsoleW((HANDLE)fhnd, global->term.buf, len, &chars_written,
                       NULL))
      return CURL_WRITEFUNC_ERROR;
  }

  *retp = bytes;
  return 0;
}
#endif /* _WIN32 */
