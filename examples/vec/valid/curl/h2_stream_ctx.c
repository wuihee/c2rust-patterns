// repos/curl/lib/http2.c

/**
 * All about the H2 internals of a stream
 */
struct h2_stream_ctx {
  struct bufq sendbuf;         /* request buffer */
  struct h1_req_parser h1;     /* parsing the request */
  struct dynhds resp_trailers; /* response trailer fields */
  size_t resp_hds_len;         /* amount of response header bytes in recvbuf */
  curl_off_t nrcvd_data;       /* number of DATA bytes received */

  // [Vec Pointer]
  char **push_headers; /* allocated array */
  // [Length]
  size_t push_headers_used; /* number of entries filled in */
  // [Capacity]
  size_t push_headers_alloc; /* number of entries allocated */

  int status_code;           /* HTTP response status code */
  uint32_t error;            /* stream error code */
  CURLcode xfer_result;      /* Result of writing out response */
  int32_t local_window_size; /* the local recv window size */
  int32_t id;                /* HTTP/2 protocol identifier for stream */
  BIT(resp_hds_complete);    /* we have a complete, final response */
  BIT(closed);               /* TRUE on stream close */
  BIT(reset);                /* TRUE on stream reset */
  BIT(reset_by_server);      /* TRUE on stream reset by server */
  BIT(close_handled);        /* TRUE if stream closure is handled by libcurl */
  BIT(bodystarted);
  BIT(body_eos);     /* the complete body has been added to `sendbuf` and
                      * is being/has been processed from there. */
  BIT(write_paused); /* stream write is paused */
};

/* frame->hd.type is either NGHTTP2_HEADERS or NGHTTP2_PUSH_PROMISE */
static int on_header(struct nghttp2_session *session,
                     const nghttp2_frame *frame, const uint8_t *name,
                     size_t namelen, const uint8_t *value, size_t valuelen,
                     uint8_t flags, void *userp) {
  struct Curl_cfilter *cf = userp;
  struct cf_h2_ctx *ctx = cf->ctx;
  struct h2_stream_ctx *stream;
  struct Curl_easy *data_s;
  int32_t stream_id = frame->hd.stream_id;
  CURLcode result;
  (void)flags;

  DEBUGASSERT(stream_id); /* should never be a zero stream ID here */

  /* get the stream from the hash based on Stream ID */
  data_s = nghttp2_session_get_stream_user_data(session, stream_id);
  if (!GOOD_EASY_HANDLE(data_s))
    /* Receiving a Stream ID not in the hash should not happen, this is an
       internal error more than anything else! */
    return NGHTTP2_ERR_CALLBACK_FAILURE;

  stream = H2_STREAM_CTX(ctx, data_s);
  if (!stream) {
    failf(data_s, "Internal NULL stream");
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  /* Store received PUSH_PROMISE headers to be used when the subsequent
     PUSH_PROMISE callback comes */
  if (frame->hd.type == NGHTTP2_PUSH_PROMISE) {
    char *h;

    if ((namelen == (sizeof(HTTP_PSEUDO_AUTHORITY) - 1)) &&
        !strncmp(HTTP_PSEUDO_AUTHORITY, (const char *)name, namelen)) {
      /* pseudo headers are lower case */
      int rc = 0;
      char *check =
          curl_maprintf("%s:%d", cf->conn->host.name, cf->conn->remote_port);
      if (!check)
        /* no memory */
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      if (!curl_strequal(check, (const char *)value) &&
          ((cf->conn->remote_port != cf->conn->given->defport) ||
           !curl_strequal(cf->conn->host.name, (const char *)value))) {
        /* This is push is not for the same authority that was asked for in
         * the URL. RFC 7540 section 8.2 says: "A client MUST treat a
         * PUSH_PROMISE for which the server is not authoritative as a stream
         * error of type PROTOCOL_ERROR."
         */
        (void)nghttp2_submit_rst_stream(session, NGHTTP2_FLAG_NONE, stream_id,
                                        NGHTTP2_PROTOCOL_ERROR);
        rc = NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      curlx_free(check);
      if (rc)
        return rc;
    }

    // [Initialize]
    if (!stream->push_headers) {
      stream->push_headers_alloc = 10;
      stream->push_headers =
          curlx_malloc(stream->push_headers_alloc * sizeof(char *));
      if (!stream->push_headers)
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      stream->push_headers_used = 0;

      // [Ensure Capacity]
    } else if (stream->push_headers_used == stream->push_headers_alloc) {
      char **headp;
      if (stream->push_headers_alloc > 1000) {
        /* this is beyond crazy many headers, bail out */
        failf(data_s, "Too many PUSH_PROMISE headers");
        free_push_headers(stream);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      stream->push_headers_alloc *= 2;
      headp = curlx_realloc(stream->push_headers,
                            stream->push_headers_alloc * sizeof(char *));
      if (!headp) {
        free_push_headers(stream);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
      }
      stream->push_headers = headp;
    }
    h = curl_maprintf("%s:%s", name, value);

    // [Push Value]
    if (h)
      stream->push_headers[stream->push_headers_used++] = h;
    return 0;
  }

  if (stream->bodystarted) {
    /* This is a trailer */
    CURL_TRC_CF(data_s, cf, "[%d] trailer: %.*s: %.*s", stream->id,
                (int)namelen, name, (int)valuelen, value);
    result = Curl_dynhds_add(&stream->resp_trailers, (const char *)name,
                             namelen, (const char *)value, valuelen);
    if (result) {
      cf_h2_header_error(cf, data_s, stream, result);
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }

    return 0;
  }

  if (namelen == sizeof(HTTP_PSEUDO_STATUS) - 1 &&
      memcmp(HTTP_PSEUDO_STATUS, name, namelen) == 0) {
    /* nghttp2 guarantees :status is received first and only once. */
    char buffer[32];
    size_t hlen;
    result = Curl_http_decode_status(&stream->status_code, (const char *)value,
                                     valuelen);
    if (result) {
      cf_h2_header_error(cf, data_s, stream, result);
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    hlen = curl_msnprintf(buffer, sizeof(buffer), HTTP_PSEUDO_STATUS ":%u\r",
                          stream->status_code);
    result = Curl_headers_push(data_s, buffer, hlen, CURLH_PSEUDO);
    if (result) {
      cf_h2_header_error(cf, data_s, stream, result);
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    curlx_dyn_reset(&ctx->scratch);
    result = curlx_dyn_addn(&ctx->scratch, STRCONST("HTTP/2 "));
    if (!result)
      result = curlx_dyn_addn(&ctx->scratch, value, valuelen);
    if (!result)
      result = curlx_dyn_addn(&ctx->scratch, STRCONST(" \r\n"));
    if (!result)
      h2_xfer_write_resp_hd(cf, data_s, stream, curlx_dyn_ptr(&ctx->scratch),
                            curlx_dyn_len(&ctx->scratch), FALSE);
    if (result) {
      cf_h2_header_error(cf, data_s, stream, result);
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    /* if we receive data for another handle, wake that up */
    if (CF_DATA_CURRENT(cf) != data_s)
      Curl_multi_mark_dirty(data_s);

    CURL_TRC_CF(data_s, cf, "[%d] status: HTTP/2 %03d", stream->id,
                stream->status_code);
    return 0;
  }

  /* nghttp2 guarantees that namelen > 0, and :status was already
     received, and this is not pseudo-header field . */
  /* convert to an HTTP1-style header */
  curlx_dyn_reset(&ctx->scratch);
  result = curlx_dyn_addn(&ctx->scratch, (const char *)name, namelen);
  if (!result)
    result = curlx_dyn_addn(&ctx->scratch, STRCONST(": "));
  if (!result)
    result = curlx_dyn_addn(&ctx->scratch, (const char *)value, valuelen);
  if (!result)
    result = curlx_dyn_addn(&ctx->scratch, STRCONST("\r\n"));
  if (!result)
    h2_xfer_write_resp_hd(cf, data_s, stream, curlx_dyn_ptr(&ctx->scratch),
                          curlx_dyn_len(&ctx->scratch), FALSE);
  if (result) {
    cf_h2_header_error(cf, data_s, stream, result);
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }
  /* if we receive data for another handle, wake that up */
  if (CF_DATA_CURRENT(cf) != data_s)
    Curl_multi_mark_dirty(data_s);

  CURL_TRC_CF(data_s, cf, "[%d] header: %.*s: %.*s", stream->id, (int)namelen,
              name, (int)valuelen, value);

  return 0; /* 0 is successful */
}
